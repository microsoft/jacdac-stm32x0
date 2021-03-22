#!/bin/sh

if [ "X`git status --porcelain --untracked-files=no`" != X ] ; then
  git status
  echo
  echo "*** You have local changes; cannot bump." 
  exit 1
fi

git pull || exit 1

eval `git describe --dirty --tags --match 'v[0-9]*' --always | sed -e 's/-.*//; s/v/v0=/; s/\./ v1=/; s/\./ v2=/'`
defl=0.0.0
if [ "X$v0" != X ] ; then
  defl=$v0.$v1.$(($v2 + 1))
fi
set -e
echo "Enter version [Enter = $defl; Ctrl-C to cancel]:"
read ver
if [ "X$ver" = "X" ] ; then
  ver="$defl"
fi

node jacdac-stm32x0/scripts/git-sublog.js -u "$ver" CHANGES.md

set -x
git add CHANGES.md
git commit -m "Automatic changelog for $ver"
git tag "$ver"
git push --tags
git push
