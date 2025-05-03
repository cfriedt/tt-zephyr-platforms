#!/usr/bin/env python3
#
# Copyright (c) 2025 Tenstorrent AI ULC
# SPDX-License-Identifier: Apache-2.0

"""Erase, write, read, and verify SPI flash via an MCU using a debug probe."""

import argparse
import sys
import struct

from elftools.elf.elffile import ELFFile
from pathlib import Path
from pyocd.core.helpers import ConnectHelper
from pyocd.core.session import Session
from pyocd.flash.loader import MemoryLoader
from pyocd.flash.flash import Flash
from pyocd.core.memory_map import (
    FlashRegion,
    MemoryType,
    MemoryMap,
    RamRegion,
)

mcu_param_cache: dict[str, dict] = {}


def get_mcu_params(elf_path: Path, bin_path: Path = None):
    """
    Get the MCU parameters for the specified MCU.
    """
    global mcu_param_cache

    if mcu_param_cache and str(elf_path) in mcu_param_cache:
        return mcu_param_cache[str(elf_path)]

    if bin_path is None:
        bin_path = Path(str(elf_path).replace(".elf", ".bin"))

    symtab = {}
    with ELFFile(elf_path.open("rb")) as elf:
        for sym in elf.get_section_by_name('.symtab').iter_symbols():
            symtab[sym.name] = sym.entry.st_value

    insns = []
    with open(bin_path, "rb") as f:
        while True:
            # Read 4 bytes
            data = f.read(4)
            if not data:
                break
            # Unpack the data as a little-endian integer
            blah = struct.unpack('<I', data)
            insns.append(blah[0])

    mcu_param = {
        "load_address": symtab["CONFIG_SRAM_BASE_ADDRESS"],
        # "load_address": symtab["z_arm_reset"],
        "sram_start": symtab["CONFIG_SRAM_BASE_ADDRESS"],
        "sram_size": symtab["CONFIG_SRAM_SIZE"] * 1024,
        "instructions": insns,
        "begin_stack": symtab["z_main_stack"] + symtab["CONFIG_MAIN_STACK_SIZE"],
        # "begin_stack": symtab["CONFIG_SRAM_BASE_ADDRESS"] + symtab["CONFIG_SRAM_SIZE"] * 1024,
        "pc_init": symtab["pyocdprog_op_init"],
        # "pc_init": symtab["z_arm_reset"],
        "pc_unInit": symtab["pyocdprog_op_uninit"],
        "pc_program_page": symtab["pyocdprog_op_program_page"],
        "pc_erase_sector": symtab["pyocdprog_op_erase_sector"],
        "pc_eraseAll": symtab["pyocdprog_op_erase_all"],
        "begin_data": symtab["pyocdprog_data"],  # what is this??
        "page_buffers": [symtab["pyocdprog_buffer"] + x * symtab["CONFIG_PYOCDPROG_BUFFER_SIZE"] for x in range(0, symtab["CONFIG_PYOCDPROG_BUFFER_COUNT"])],
        # "static_base": symtab["pyocdprog_static_base"],
        "static_base": 0,
        "while_loop": symtab["pyocdprog_while_loop"],
    }

    mcu_param_cache[str(elf_path)] = mcu_param
    return mcu_param

SPI_FLASH_PARAMS = {
    "w25q16jv": {
        "size": 0x200000,  # 2 MiB
        "sector_size": 0x1000,  # 4 KiB
        "page_size": 0x100,  # 256 B
    },
}


def test_img(page_size: int = 256):
    """
    Generate a test image for the specified page size.
    """
    if (page_size <= 32) or ((page_size - 1) & page_size != 0):
        raise ValueError("Page size must be a positive power of 2 >= 32")

    return [0x55 if (x % 2) else 0xAA for x in range(0, page_size)]


def sram_pages(mcu: str, flash: str, elf: Path):
    """
    Generate a list of SRAM pages for the specified MCU.
    """
    mcu_params = get_mcu_params(elf)
    sram_start = mcu_params["sram_start"]
    sram_size = mcu_params["sram_size"]

    if flash not in SPI_FLASH_PARAMS:
        raise ValueError(f"Unsupported flash part: {flash}")

    flash_params = SPI_FLASH_PARAMS[flash]
    flash_sector_size = flash_params["sector_size"]

    return [sram_start + x for x in range(0, sram_size, flash_sector_size)]


def get_spi_flash_algorithm(mcu: str, flash: str, elf: Path):
    """
    Generate a SPI flash algorithm for the specified mcu and flash part.
    """
    mcu_params = get_mcu_params(elf)
    sram_start = mcu_params["sram_start"]
    sram_size = mcu_params["sram_size"]
    stack_start = sram_start + sram_size

    if flash not in SPI_FLASH_PARAMS:
        raise ValueError(f"Unsupported flash part: {flash}")

    flash_params = SPI_FLASH_PARAMS[flash]
    flash_page_size = flash_params["page_size"]

    sram_page = sram_pages(mcu, flash, elf)

    return {
        "load_address": mcu_params["load_address"],
        "instructions": mcu_params["instructions"],
        "pc_init": mcu_params["pc_init"],
        "pc_unInit": mcu_params["pc_unInit"],
        "pc_program_page": mcu_params["pc_program_page"],
        "pc_erase_sector": mcu_params["pc_erase_sector"],
        "pc_eraseAll": mcu_params["pc_eraseAll"],
        "static_base": mcu_params["static_base"],
        "begin_stack": mcu_params["begin_stack"],
        "begin_data": mcu_params["begin_data"],
        "page_size": flash_page_size,
        "analyzer_supported": False,
        "analyzer_address": 0,  # sram_page[3],
        # "analyzer_address": sram_start + mcu_params["pc_compute_crc"],
        "page_buffers": mcu_params["page_buffers"],
        "min_program_length": flash_page_size,
    }


def get_memory_map(mcu: str, flash: str, elf: Path):
    """
    Generate a memory map for the specified MCU and flash part.
    """
    mcu_params = get_mcu_params(elf)
    sram_start = mcu_params["sram_start"]
    sram_size = mcu_params["sram_size"]

    if flash not in SPI_FLASH_PARAMS:
        raise ValueError(f"Unsupported flash part: {flash}")

    flash_params = SPI_FLASH_PARAMS[flash]
    flash_size = flash_params["size"]
    flash_sector_size = flash_params["sector_size"]
    flash_page_size = flash_params["page_size"]

    return MemoryMap(
        FlashRegion(
            start=0,
            length=flash_size,
            page_size=flash_page_size,
            blocksize=flash_sector_size,
            is_boot_memory=False,
            erased_byte_value=0xFF,
            algo=get_spi_flash_algorithm(mcu, flash, elf),
            sector_size=flash_sector_size,
        ),
        RamRegion(
            start=sram_start,
            length=sram_size,
        ),
    )


class SpiLoader(MemoryLoader):
    """
    SPI Programmer class for programming external SPI flash via BMC.
    """

    def __init__(self, session: Session, mcu: str, flash: str, elf: Path):
        super().__init__(session)
        self._progress = self.progress_callback

        memory_map = get_memory_map(mcu, flash, elf)
        algo = get_spi_flash_algorithm(mcu, flash, elf)

        session.board.target.memory_map = memory_map

        self._map = session.board.target.memory_map
        self._flash = Flash(session.board.target, algo)
        # self._flash.set_flash_algo_debug(True)

        for region in self._map:
            if region.type == MemoryType.FLASH:
                region.flash = self._flash
                self._flash.region = region
                break

    def progress_callback(self, progress: float) -> None:
        """
        Callback function to report progress.
        """
        print(f"Progress: {progress * 100:.2f}%")


def main():
    # For Debugging Purposes only
    if not Path("/tmp/blah.bin").exists():
        with open("/tmp/blah.bin", "wb") as f:
            f.write(bytes(test_img(256)))
    sys.argv[1:] = ["0x0", "/tmp/blah.bin"]

    try:
        args = parse_args()

        options = {}
        if args.chip_erase:
            options["chip_erase"] = args.chip_erase
        if args.keep_unwritten:
            options["keep_unwritten"] = args.keep_unwritten
        if args.no_reset:
            options["no_reset"] = args.no_reset
        if args.smart_flash:
            options["smart_flash"] = args.smart_flash
        if args.trust_crc:
            options["trust_crc"] = args.trust_crc
        if args.target_override:
            options["target_override"] = args.target_override
        if args.unique_id:
            options["unique_id"] = args.unique_id

        mcu: str = args.target_override
        flash: str = args.flash_override

        # Connect to the target device
        with ConnectHelper.session_with_chosen_probe(
            unique_id=args.unique_id,
            auto_open=True,
            options=options,
        ) as session:
            session.target.reset_and_halt()

            programmer = SpiLoader(session, mcu, flash, args.elf)

            for input_tuple in args.input_tuples:
                addr = input_tuple["addr"]
                data = input_tuple["data"]
                programmer.add_data(addr, data)
            programmer.commit()

    except Exception as e:
        print(f"An error occurred: {e}")


def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser(
        description="Erase, write, read, and verify SPI flash via an MCU using a debug probe",
        allow_abbrev=False,
    )
    parser.add_argument(
        "--chip-erase",
        "-e",
        action="store_true",
        help="Target override for the session.",
    )
    parser.add_argument(
        "--flash-override",
        "-f",
        type=str,
        default="w25q16jv",
        help="spi flash part",
    )
    parser.add_argument(
        "--keep-unwritten",
        "-k",
        action="store_true",
        help="Keep unwritten pages in flash.",
    )
    parser.add_argument(
        "--elf",
        "-l",
        type=Path,
        help="ELF file for flashing algorithm (pyocdprog).",
        default=Path("build/zephyr/zephyr.elf"),
    )
    parser.add_argument(
        "--no-reset",
        "-n",
        action="store_true",
        help="Do not reset the target after programming.",
    )
    parser.add_argument(
        "--smart-flash",
        "-s",
        action="store_true",
        help="Use smart flash programming.",
    )
    parser.add_argument(
        "--trust-crc",
        "-r",
        action="store_true",
        default=False,
        help="Use the sector crc to determine if the sector already contains the data to be flashed.",
    )
    parser.add_argument(
        "--target-override",
        "-t",
        type=str,
        default="stm32l496zgtx",
        help="pyocd pack name / target override for the session.",
    )
    parser.add_argument(
        "--unique-id",
        "-u",
        type=str,
        default="260024000A2D343632525544",
        help="Unique ID of the debug adapter.",
    )
    parser.add_argument(
        "--verify",
        "-v",
        action="store_true",
        help="Verify the flash after programming.",
    )
    parser.add_argument(
        metavar="ADDR FILENAME",
        dest="input_tuples",
        type=str,
        action="append",
        nargs=2,
        default=[],
        help="An address followed by a path to a binary file",
    )

    args = parser.parse_args()

    # A possibly better alternative for our use case would be to have another optional parameter
    # "-f FILENAME", "--tt-boot-fs FILENAME", that would be mutually exclusive with input tuples.
    # With that we could dynamically construct regions to flash base on the tt-boot-fs image
    # directly. Only 1 tt-boot-fs parameter though, as opposed to mulitiple input tuples.
    input_tuples = []
    for tuples in args.input_tuples:
        addr = int(tuples[0], 0)
        mpath = Path(tuples[1])
        if not mpath.is_file():
            raise FileNotFoundError(f"File not found: {mpath}")
        data = mpath.read_bytes()
        input_tuples.append({"addr": addr, "data": data})
    args.input_tuples = input_tuples

    # expand this later to check for either input_tuples or tt_boot_fs
    if not args.input_tuples:
        parser.error("At least one address and filename pair is required.")

    return args


if __name__ == "__main__":
    main()
