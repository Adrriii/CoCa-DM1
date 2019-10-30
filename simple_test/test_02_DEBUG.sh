#!/bin/bash

../equalPath -Dsa ../graphs/assignment-instance/G1.dot;
echo "must be ->";
../working-equalPath -s -a ../graphs/assignment-instance/G1.dot;

