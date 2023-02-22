#! /bin/bash


#submission_dir="$(pwd)"

# docker run --rm -it -v $(pwd):/opt baseline
# cd /opt
# make

make
./bin/init tmp/foo
./bin/router &
./bin/bank tmp/foo.bank &
./bin/atm tmp/foo.atm


# docker run --name test_cont --rm -d -v "${submission_dir}:/opt" -w /opt baseline make -C /opt
# docker exec -u root test_cont python3 public2.py $(pwd)

# docker logs test_cont

# docker stop test_cont
# docker rm test_cont