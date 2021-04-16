#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# File            : build_mbed.py
# Author          : Duy Anh Pham <duyanh.y4n.pham@gmail.com>
# Date            : 13.10.2020
# Last Modified By: Duy Anh Pham <duyanh.y4n.pham@gmail.com>
from enum import Enum
import os
import shutil
import fire
from loguru import logger
import glob

COMMAND = 'mbed'

TARGET_BOARD = [
    'NUCLEO_F767ZI',
    'DISCO_F769NI',
]

TOOLCHAIN = [
    'GCC_ARM',
    'ARM'
]

PROFILE = [
    'debug'
]

BUILD_DIR_DEFAULT = 'BUILD'

TO_BE_CLEANED = '.to_be_cleaned'
TO_BE_CLEANED_DEFAULT_CONTENT = '# This is a comment\n# Default build dir is BUILD \nmain.*\n'

logger.add(os.sys.stdout,
           format="{time} {level} {message}", filter="my_module", level="INFO")
LOG = False


def mbed_compile(target_board,
                 src='.',
                 build_dir=BUILD_DIR_DEFAULT,
                 profile='debug',
                 tool_chain='GCC_ARM',
                 log=LOG,
                 verbose=False,
                 ):
    config_log(verbose, log)
    if target_board not in TARGET_BOARD:
        raise Exception(
            'Board not yet supported, choose one from ' + str(TARGET_BOARD))
    if tool_chain not in TOOLCHAIN:
        raise Exception(
            'TOOLCHAIN not yet supported, choose one from ' + str(TOOLCHAIN))
    if profile not in PROFILE:
        raise Exception(
            'profile not yet supported, choose one from ' + str(PROFILE))
    cmd = (COMMAND + " compile -j4 -t {} -m ${} --source {} --build {} --profile {}".format(
        tool_chain, target_board, src, build_dir, profile))
    run_cmd(cmd, verbose=verbose)


def export_makefile(target_board,
                    src='.',
                    build_export_dir='.',
                    profile='debug',
                    tool_chain='GCC_ARM',
                    log=LOG,
                    verbose=False
                    ):
    config_log(verbose, log)
    if target_board not in TARGET_BOARD:
        raise Exception(
            'Board not yet supported, choose one from ' + str(TARGET_BOARD))
    if tool_chain not in TOOLCHAIN:
        raise Exception(
            'TOOLCHAIN not yet supported, choose one from ' + str(TOOLCHAIN))
    if profile not in PROFILE:
        raise Exception(
            'profile not yet supported, choose one from ' + str(PROFILE))
    cmd = (COMMAND + ' export -i make_gcc_arm -m {} --source {} --build {}'.format(
        target_board, src, build_export_dir))
    run_cmd(cmd, verbose=verbose)


def run_cmd(cmd, verbose=True):
    logger.debug(cmd)
    if not verbose:
        os.system(cmd)


def clean(clear_all=False,
          verbose=False,
          log=LOG,
          build_dir=BUILD_DIR_DEFAULT):
    config_log(verbose, log)
    logger.debug(os.sys.argv[0])
    try:
        to_be_cleaned_file = open(TO_BE_CLEANED, 'r')
    except FileNotFoundError:
        logger.warning(TO_BE_CLEANED + ' not found -> creating default file')
        to_be_cleaned_file = open(TO_BE_CLEANED, 'w')
        to_be_cleaned_file.writelines(TO_BE_CLEANED_DEFAULT_CONTENT)
        to_be_cleaned_file.close()
        to_be_cleaned_file = open(TO_BE_CLEANED, 'r')
    to_be_cleaned_list_unfiltered = to_be_cleaned_file.readlines()
    to_be_cleaned_list = list()
    for i, pattern in enumerate(to_be_cleaned_list_unfiltered): # filter out empty line and comments
        to_be_cleaned_list_unfiltered[i] = pattern.strip()
        if len(to_be_cleaned_list_unfiltered[i]) > 0 and to_be_cleaned_list_unfiltered[i][0] != '#':
            to_be_cleaned_list.append(to_be_cleaned_list_unfiltered[i])
    remove_files(build_dir, to_be_cleaned_list, verbose)


def remove_files(path_to_be_cleaned, to_be_cleaned_list, verbose):
    logger.debug('To be cleaned pattern ' + str(to_be_cleaned_list))
    for pattern in to_be_cleaned_list:
        for matched in glob.glob(os.path.join(path_to_be_cleaned, pattern)):
            if os.path.isdir(os.path.join(matched)):
                logger.debug('Removing directory:' + os.path.join(matched))
                if not verbose:
                    shutil.rmtree(_to_be_deleted, ignore_errors=False)
            else:
                logger.debug('Removing file:' + os.path.join(matched))
                if not verbose:
                    os.remove(os.path.join(matched))


def config_log(verbose, log):
    if not log and not verbose:
        logger.remove()


if __name__ == "__main__":
    fire.Fire({
        'compile': mbed_compile,
        'Makefile': export_makefile,
        'clean': clean,
    })
