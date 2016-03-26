#/bin/sh

export LC_ALL=C
COMPILER=$1

check() {
  error=0
  output=`echo "int d;" | $COMPILER -xc++ $1 -c -S -o- - 2>&1 1>/dev/null`
  rv=$?
  case $output in
    *warning*|*error*|*invalid* ) error=1 ;;
  esac
  test $rv -eq 0 && test $error -eq 0 && echo "$1" && test -n "$F" && exit 0
}

for f in `echo $@ | cut -d' ' -f2-`; do
  check $f
done
