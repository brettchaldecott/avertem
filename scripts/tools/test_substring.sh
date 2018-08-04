#!/bin/bash

value=$1

value=${value#*keto_}
echo "${value}"
