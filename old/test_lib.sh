check(){
  local exp_err=$1; shift;
  local msg=$1; shift;

  echo ">> ./eventdb $@ -- $msg"
  ./eventdb "$@" 2>&1
  local ret=$?

  echo "Result got/expected: $ret/$exp_err"

  if [ "$exp_err" = 1 -a "$ret"  = 0 ] ||
     [ "$exp_err" = 0 -a "$ret" != 0 ]; then
    echo "WRONG RESPONSE!"
    exit
  fi
  echo
}
