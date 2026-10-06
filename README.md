# Эмулятор командной строки — вариант 9 (этапы 1–3)

Linux/macOS, `g++` (C++17), `make`:

```bash
make
./emulator --vfs examples/deep.xml --script examples/vfs.txt
./examples/config.sh
./examples/errors.sh
./examples/vfs.sh
```

Этап 1: REPL, приглашение из данных ОС, заглушки `ls`, `cd`, команда `exit`.
Этап 2: параметры `--vfs`, `--script`, команда `conf-dump`, стартовый скрипт с комментариями `//`.
Этап 3: XML VFS загружается в память; `vfs-dump` показывает её дерево. `ls` и `cd` пока остаются заглушками (этап 4).

Формат XML: корень `<vfs>`, внутри `<dir name="...">` и `<file name="...">текст</file>`.
Для двоичного файла укажите `encoding="base64"`; XML сущности `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;` поддерживаются. Имена внутри папки уникальны.
Если `--vfs` не задан, работать без VFS можно, но `vfs-dump` сообщает об ошибке.

`examples/minimal.xml`, `files.xml`, `deep.xml` показывают три варианта VFS.
`examples/invalid.xml` проверяет ошибку формата. Скрипт `vfs.sh` запускает разные варианты VFS, а `errors.sh` проверяет отсутствие файла и ошибки XML.
Ошибки внутри `vfs.txt` намеренные: скрипт продолжает работу и показывает номер строки.

Сценарий этапа 1: `./emulator`, затем `ls`, `cd docs`, `ls a b`, `unknown`, `exit`.
