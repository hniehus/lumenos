#!/usr/bin/env bash
set -euo pipefail

version_file=$1
counter_file=$2
header_output=$3
text_output=$4

. "./${version_file}"

mkdir -p "$(dirname "${header_output}")"
mkdir -p "$(dirname "${text_output}")"

current=0
if [ -f "${counter_file}" ]; then
  current=$(tr -d '[:space:]' < "${counter_file}")
fi

next=$((10#${current} + 1))
build=$(printf '%05d' "${next}")
version="${MAJOR}.${MINOR}.${PATCH}:${build}"

printf '%s\n' "${next}" > "${counter_file}"
cat > "${header_output}" <<EOF
#ifndef LUMEN_VERSION_H
#define LUMEN_VERSION_H

#define LUMEN_VERSION_MAJOR ${MAJOR}
#define LUMEN_VERSION_MINOR ${MINOR}
#define LUMEN_VERSION_PATCH ${PATCH}
#define LUMEN_VERSION_BUILD "${build}"
#define LUMEN_VERSION_STRING "${version}"

#endif
EOF
printf '%s\n' "${version}" > "${text_output}"
