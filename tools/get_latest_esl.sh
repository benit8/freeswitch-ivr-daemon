#!/usr/bin/env bash
set -e

dir=$(cd "$(dirname "$0")" ; pwd)
root=$(dirname "$dir")

version="1.10.7"
release_url="https://github.com/signalwire/freeswitch/archive/refs/tags/v${version}.zip"
libesl_path="freeswitch-${version}/libs/esl/src"
archive_filename="freeswitch.zip"
output_path="$root/include/esl"

# Download the release
wget -qc "$release_url" -O "$archive_filename"

# Only extract the ESL from it
unzip -qj "$archive_filename" "$libesl_path/*" -d "$output_path/"

# Get rid of the archive
rm -rf "$archive_filename"
