#!/bin/bash

../equalPath -Dsa ../graphs/assignment-instance/square.dot;
echo "must be ->";
../working-equalPath -s -a ../graphs/assignment-instance/square.dot;

