# Fix high CPU usage

## Visualize in a frame graph

```console
./update-libspringql_client.sh 'v0.16.0+3'
```

```console
./build-run.sh  # run main.c
```

```console
top  # check CPU usage
```

See: <https://yohei-a.hatenablog.jp/entry/20150706/1436208007>

```console
perf record -g -F100000 -p $(pidof a.out)
perf script > perf_data.txt
perl stackcollapse-perf.pl perf_data.txt|perl flamegraph.pl --width 5000 --title "Flame Graphs - ./a.out" > flamegraph_aout.svg
```

## Thoughts

- SourceWorker consumes about 70% of CPU when no source row is incoming.
