export PYTHONDONTWRITEBYTECODE=1
for py in 8 7 6
do
    if which python3.${py}
    then
        export PYTHON=python3.${py}
        break
    fi
done
echo Will use python $PYTHON

$PYTHON -u -B -m modgen

