#!/bin/bash

args=("$@")
for ((i=0; i<"${#args[@]}"; ++i)); do
    case ${args[i]} in
	-fext-numeric-literals) unset args[i]; break;;
    esac
done

clang-tidy-11 "${args[@]}"
