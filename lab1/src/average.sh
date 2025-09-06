#!/bin/bash

sum=0
count=0

# Перебираем все аргументы
for num in "$@"; do
    sum=$((sum + num))
    count=$((count + 1))
done

average=$((sum / count))

echo "Количество: $count"
echo "Среднее: $average"
