#!/usr/bin/env bash
########################################
set -e
########################################
readonly RELDIR="$(dirname ${0})"
readonly INCDIR="${RELDIR}/../include"
readonly OUTDIR="${RELDIR}/../out"
########################################

####################
eprintln() {
	! [ -z "${1}" ] || eprintln 'eprintln: expected <msg>'
	printf "${1}\n" 1>&2
	return 1
}

####################
run() {
	! [ -z "${1}" ] || \
	eprintln 'expected: <file.h> [flags]'

	[ -e "${OUTDIR}" ] || mkdir -p "${OUTDIR}"

	! [ -e "${OUTDIR}/hcheck" ] || rm "${OUTDIR}/hcheck"

	cat <<EOF>"${OUTDIR}/hcheck.c"
#include "${1}"

int main(void)
{
	return 0;
}
EOF

	local output="$(cc -v -std=c89 -pedantic -O0 -o "${OUTDIR}/hcheck" -I${INCDIR} -ferror-limit=0 ${2} "${OUTDIR}/hcheck.c" 2>&1)"

	cc -v -std=c89 -pedantic -O0 -o "${OUTDIR}/hcheck" -I${INCDIR} \
	-ferror-limit=0 ${2} "${OUTDIR}/hcheck.c" 2>&1 | \
	sed 's/.*error:/error:/g' | \
	sort -u | \
	grep 'error:' && return 1 || true
	
	./"${OUTDIR}/hcheck" && printf "PASS\n" || printf "FAIL\n"
}

####################
verb() {
	! [ -z "${1}" ] || \
	eprintln 'expected: <file.h> [flags]'

	[ -e "${OUTDIR}" ] || mkdir -p "${OUTDIR}"

	! [ -e "${OUTDIR}/hcheck" ] || rm "${OUTDIR}/hcheck"

	cat <<EOF>"${OUTDIR}/hcheck.c"
#include "${1}"

int main(void)
{
	return 0;
}
EOF

	cc -std=c89 -pedantic -O0 -o "${OUTDIR}/hcheck" -I${INCDIR} \
	-ferror-limit=0 ${2} "${OUTDIR}/hcheck.c" 2>&1 || return 1

	./"${OUTDIR}/hcheck" && printf "PASS\n" || printf "FAIL\n"
}

####################
link()
{
	! [ -z "${1}" ] || \
	eprintln 'expected: <file.h> [flags]'

	[ -e "${OUTDIR}" ] || mkdir -p "${OUTDIR}"

	! [ -e "${OUTDIR}/hcheck" ] || rm "${OUTDIR}/hcheck"

	cat <<EOF>"${OUTDIR}/hcheck.c"
#include "${1}"

int main(void)
{
	return 0;
}
EOF

	cc -std=c89 -pedantic -O0 -o "${OUTDIR}/hcheck" -I${INCDIR} \
	-ferror-limit=0 ${2} "${OUTDIR}/hcheck.c" 2>&1 | \
	grep 'referenced\ from' && return 1 || true

	./"${OUTDIR}/hcheck" && printf "PASS\n" || printf "FAIL\n"
}

########################################
case ${1} in
	compile) run "${2}" "${3}" ;;
	verbose) verb "${2}" "${3}" ;;
	link) link "${2}" "${3}" ;;
	*) eprintln "usage: < compile | verbose | link >"
esac
