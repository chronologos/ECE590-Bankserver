# Loadtesting
## Loadtesting instructions
    $ pip3 install lxml
    $ python3 load-test.py localhost

## Loadtesting details
1. load-test sends `primer.xml` to create accounts.
2. after sufficient time, load-test spins up multiple threads and starts sending transactions on accounts
3. after sufficient time, load-test sends `checkres.xml` and checks the result against the local result in `checkres.xml`.
