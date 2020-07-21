#! /usr/bin/env python3

# SPDX-FileCopyrightText: 2014 MicroPython & CircuitPython contributors (https://github.com/adafruit/circuitpython/graphs/contributors)
#
# SPDX-License-Identifier: MIT

import multiprocessing
import pathlib
import os
import sys
import subprocess
import shutil
import build_board_info as build_info
import time

BASEDIR = pathlib.Path(__file__).parent.parent
sys.path.append(BASEDIR / "docs")

for port in build_info.SUPPORTED_PORTS:
    result = subprocess.run("rm -rf {BASEDIR}/ports/{port}/build*".format(BASEDIR=BASEDIR,port=port), shell=True)

PARALLEL = "-j{}".format(multiprocessing.cpu_count())

all_boards = build_info.get_board_mapping()
build_boards = list(all_boards.keys())
if "BOARDS" in os.environ:
    build_boards = os.environ["BOARDS"].split()

sha, version = build_info.get_version_info()

languages = build_info.get_languages()
exit_status = 0
for board in build_boards:
    bin_directory = BASEDIR / "bin" / board
    os.makedirs(bin_directory, exist_ok=True)
    board_info = all_boards[board]
    port = board_info["port"]
    arch = build_info.arch_by_port[port]
    portdir=BASEDIR / "ports" / port

    for language in languages:
        bin_directory = BASEDIR / "bin" / board / language
        os.makedirs(bin_directory, exist_ok=True)
        start_time = time.monotonic()
        # Normally different language builds are all done based on the same set of compiled sources.
        # But sometimes a particular language needs to be built from scratch, if, for instance,
        # CFLAGS_INLINE_LIMIT is set for a particular language to make it fit.
        clean_build_check_result = subprocess.run(
            "make -C {portdir} TRANSLATION={language} BOARD={board} check-release-needs-clean-build | fgrep 'RELEASE_NEEDS_CLEAN_BUILD = 1'".format(
                portdir=portdir, language=language, board=board),
            shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        clean_build = clean_build_check_result.returncode == 0

        build_dir = "build-{board}".format(board=board)
        if clean_build:
            build_dir += "-{language}".format(language=language)

        make_result = subprocess.run(
            "make {PARALLEL} -C {portdir} TRANSLATION={language} BOARD={board} BUILD={build}".format(
                PARALLEL=PARALLEL, portdir=portdir, arch=arch, port=port, language=language, board=board, build=build_dir),
            shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        build_duration = time.monotonic() - start_time
        success = "\033[32msucceeded\033[0m"
        if make_result.returncode != 0:
            exit_status = make_result.returncode
            success = "\033[31mfailed\033[0m"

        other_output = ""

        for extension in board_info["extensions"]:
            temp_filename = portdir / "{build}/firmware.{extension}".format(
                port=board_info["port"], build=build_dir, extension=extension)
            for alias in board_info["aliases"] + [board]:
                bin_directory = BASEDIR / "bin/{alias}/{language}".format(
                    alias=alias, language=language)
                os.makedirs(bin_directory, exist_ok=True)
                final_filename = "adafruit-circuitpython-{alias}-{language}-{version}.{extension}".format(
                    alias=alias, language=language, version=version, extension=extension)
                final_filename = os.path.join(bin_directory, final_filename)
                try:
                    shutil.copyfile(temp_filename, final_filename)
                except FileNotFoundError:
                    other_output = "Cannot find file {}".format(temp_filename)
                    if exit_status == 0:
                        exit_status = 1

        print("Build {board} for {language}{clean_build} took {build_duration:.2f}s and {success}".format(
            board=board, language=language, clean_build=(" (clean_build)" if clean_build else ""),
            build_duration=build_duration, success=success))

        print(make_result.stdout.decode("utf-8"))
        print(other_output)

        # Flush so we will see something before 10 minutes has passed.
        print(flush=True)

sys.exit(exit_status)
