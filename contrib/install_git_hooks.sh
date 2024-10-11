#!/bin/sh

# SPDX-License-Identifier: GPL-2.0
#
# Install git hooks
#

scriptname="${0##*/}"

set -e

BASE_REPO_DIR="$(git rev-parse --show-toplevel 2>/dev/null)"
if [ $? -ne 0 ]
then
	printf '%s: Error! not in a git clone or repo.\n'  "${scriptname}"
	exit 1
fi
if [ ! -d "${BASE_REPO_DIR}/.git/hooks" ]
then
	printf '%s: Error! .git/hooks/ not found.\n'  "${scriptname}"
	exit 1
fi

cat >"${BASE_REPO_DIR}/.git/hooks/pre-commit" <<'EOI'
#!/bin/sh

scriptname="${0##*/}"

errval=0

./contrib/style.sh
if [ $? -ne 0 ] ; then
	errval=1
fi

exit ${errval}
EOI
chmod +x "${BASE_REPO_DIR}/.git/hooks/pre-commit"

printf '%s: The pre-commit git hook was installed successfully.\n'\
	"${scriptname}"
exit 0
