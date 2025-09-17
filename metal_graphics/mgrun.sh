#!/bin/sh

t=$(tty)

open -W --stdin $t --stdout $t --stderr $t $(find . -type d -name "mg.app")