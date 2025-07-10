#!/usr/bin/env python3

# Copyright (c) 2025 Tenstorrent AI ULC
# SPDX-License-Identifier: Apache-2.0

"""
Script to sync tagged artifacts from tt-firmware repository to a target repository.
This script uses the GitHub API to fetch tagged artifacts from the tt-firmware repository
and push them to a specified target repository.

Usage:
    python ttf-sync.py -o <owner> -r <repo> -t <token> [-b <branch>] [-d <date>]
"""

import argparse
import logging
import os
import sys

from datetime import datetime, timedelta, timezone
from github import Auth
from github import Github
from pathlib import Path

TTF_OWNER = "tenstorrent"
TTF_REPO = "tt-firmware"
TTF_URL = f"https://github.com/{TTF_OWNER}/{TTF_REPO}.git"

DEFAULT_DATE = datetime.strptime("2025-05-01", "%Y-%m-%d").replace(tzinfo=timezone.utc)

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def valid_date_type(arg_date_str):
    try:
        return datetime.strptime(arg_date_str, "%Y-%m-%d").replace(tzinfo=timezone.utc)
    except ValueError:
        msg = "Given Date ({0}) not valid! Expected format, YYYY-MM-DD!".format(arg_date_str)
        raise argparse.ArgumentTypeError(msg)

def parse_args():
    parser = argparse.ArgumentParser(description="Sync tagged artifacts from tt-firmware", allow_abbrev=False)

    # A branch will be created in the target repository based on the root commit (i.e. an empty
    # repo with no history). This branch will be used to make commits that are tied to the
    # synchronized artifacts. Note: we don't actually need to check binaries into git. All we need
    # is a git commit-ish to tag from from which we can generate a release. Perhaps all that will
    # be needed in this branch is a text file that lists the artifacts, the sha256 checksums, and
    # the original URL from which the artifacts were downloaded.
    parser.add_argument(
        "-b",
        "--branch",
        type=str,
        default="legacy",
        metavar="BRANCH",
        help="Name of new branch to create in the target repository.",
    )

    # This date represents the first tagged release (or release candidate) from the target
    # repository that includes both Grayskull and Wormhole binary blobs. Thus, any tagged
    # releases made after this date do not need to be synchronized. Only tagged releases made
    # before would need to be synchronized). Additionally, "experimental builds" identified
    # by (sha256,string) pairs in the tt-firmware repository must be synchronized. A separate
    # list of experimental builds is supplied via the `--experimental` argument.
    parser.add_argument(
        "-d",
        "--date",
        type=valid_date_type,
        default=f"{DEFAULT_DATE.strftime('%Y-%m-%d')}",
        metavar="DATE",
        help=f"Date for which previously published artifacts should be synced (default: {DEFAULT_DATE.strftime('%Y-%m-%d')}).",
    )

    parser.add_argument(
        "-e",
        "--experimental",
        type=Path,
        metavar="FILE",
        help="JSON file containing experimental builds to sync (identified by sha256 and string).",
    )

    parser.add_argument(
        "-o",
        "--org",
        type=str,
        required=True,
        metavar="ORGANIZATION",
        help="Organization (or owner) of the target GitHub repository.",
    )

    parser.add_argument(
        "-r",
        "--repo",
        type=str,
        required=True,
        metavar="REPOSITORY",
        help="Target repository name.",
    )

    # Requires a personal access token with "Contents: Read and Write" permissions.
    parser.add_argument(
        "-t",
        "--token",
        type=Path,
        metavar="FILE",
        dest='tokenfile',
        help='File containing GitHub token (alternatively, use GITHUB_TOKEN env variable)',
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action='store_true',
        help='Increase verbosity of logging output.',
    )

    args = parser.parse_args()

    if args.tokenfile:
        with open(args.tokenfile, 'r') as file:
            token = file.read()
            token = token.strip()
    else:
        if 'GITHUB_TOKEN' not in os.environ:
            raise ValueError('No credentials specified')
        token = os.environ['GITHUB_TOKEN']
    setattr(args, 'token', token)

    return args

def main() -> int:
    try:
        args = parse_args()
    except Exception as e:
        logger.error('failed to parse arguments: %s', e)
        return os.EX_USAGE

    if args.verbose:
        logger.setLevel(logging.DEBUG)

    try:
        gh = Github(args.token)
    except Exception as e:
        logger.error('failed to authenticate with GitHub')
        return os.EX_DATAERR
    
    try:
        source_repo = gh.get_repo(f"{TTF_OWNER}/{TTF_REPO}")
    except Exception:
        logger.error(f'failed to obtain the source Github repository {TTF_OWNER}/{TTF_REPO}')
        return os.EX_DATAERR

    try:
        target_repo = gh.get_repo(args.org + '/' + args.repo)
    except Exception:
        logger.error('failed to obtain Github repository %s/%s', args.org, args.repo)
        logger.error('does the provided github token include repository access and "contents: read and write" permissions?')
        return os.EX_DATAERR

    logger.info(f'fetching tags from source repository {TTF_OWNER}/{TTF_REPO} before {datetime.strftime(args.date, "%Y-%m-%d")}.')
    source_tags = set()
    for tag in source_repo.get_tags():
        if tag.commit.commit.committer.date >= args.date:
            logger.debug(f'skipping tag {tag.name} as it is already included in the target repository')
            continue
        logger.debug(f'found tag: {tag}')
        source_tags.add(tag.name)
    logger.info(f'found tags: {source_tags}')

    logger.info(f'fetching tags from target repository {args.org}/{args.repo}')
    target_tags = set()
    for tag in target_repo.get_tags():
        if tag.commit.commit.committer.date >= args.date:
            logger.debug(f'skipping tag {tag.name} as it is already included in the source repository')
            continue
        logger.debug(f'found tag: {tag}')
        target_tags.add(tag.name)
    logger.info(f'found tags: {target_tags}')

    overlap_tags = source_tags.intersection(target_tags)
    if overlap_tags:
        logger.error(f'found overlapping tags: {overlap_tags}')
        return os.EX_DATAERR

    return 0


if __name__ == "__main__":
    sys.argv = [sys.argv[0], '-t', f'{os.environ['HOME']}/.ttftoken', '-o', 'cfriedt', '-r', 'tt-zephyr-platforms']
    sys.exit(main())
