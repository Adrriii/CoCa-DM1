#!/bin/bash

../equalPath -sa ../graphs/generic-instances/positive-instances/Taille10_pos/instance1/G1.dot ../graphs/generic-instances/positive-instances/Taille10_pos/instance1/G2.dot ../graphs/generic-instances/positive-instances/Taille10_pos/instance1/G3.dot;
echo "must find ->";
../working-equalPath -s -a ../graphs/generic-instances/positive-instances/Taille10_pos/instance1/G1.dot ../graphs/generic-instances/positive-instances/Taille10_pos/instance1/G2.dot ../graphs/generic-instances/positive-instances/Taille10_pos/instance1/G3.dot
