## Testdata

Singular entrypoint for testdata creation.

### To run:

1. Make sure testdata directory is on Path

```bash
export PATH="/[your_path]/testdata/:$PATH"
```

2. Make sure you have required libraries installed

```bash
cd testdata
pip install -r requirements-dev.txt
```

3. Run entry script and supply all the required arguments

```bash
cd testdata
python make_testdata.py [your arguments]
```

Get help if needed

```bash
python make_testdata.py -h
```

### To extend:

Modules implementation is conceptually independent from each other, so each of
the modules is structured to best represent the testdata cases it requires. If
current structure is too rigid for the new coming testdata case, dismantle it.

To implement totally new module, extend and implement `common.Category` abstract
class and add an instance to `categories` in `make_testdata.py`.
