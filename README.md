# Эмулятор UNIX-подобной командной строки — вариант 9, этапы 1–2

## Сборка и запуск

Нужны Linux/macOS, `g++` (C++17), `make` и Git. В каталоге проекта:

```bash
make
./emulator
```


## Этап 1

По очереди:

```text
ls
cd docs
ls a b
unknown
exit extra
exit
```
## Этап 2

```bash
./emulator --vfs examples/future-vfs.xml --script examples/config.txt
./examples/config.sh
./examples/errors.sh
```



История проекта содержит ровно два коммита, по одному на этап:

```bash
git log --oneline --reverse
```
