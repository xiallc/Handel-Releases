#!/bin/bash
set -e
HAMMER="./swtoolkit/hammer.sh"
TMP=/tmp ${HAMMER} -j4 --verbose --samples --udxp --no-udxps --no-xw --no-serial --no-xmap --no-stj $*