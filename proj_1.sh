#!/bin/bash

. ./activate
cd threads/build
pintos -- run alarm-multiple > log