#!/bin/bash

echo "Текущий путь: $PWD"
echo "Текущая дата и время: $(date '+%Y-%m-%d %H:%M:%S')"
echo "Переменная PATH:"
echo "$PATH" | tr ':' '\n'