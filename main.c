#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "springql.h"

void abort_with_report()
{
    SpringErrno errno_;
    char errmsg[1024];
    spring_last_err(&errno_, errmsg, 1024);
    fprintf(stderr, "Error occurred (%d): %s", errno_, errmsg);
    abort();
}

void assert_ok(SpringErrno ret)
{
    if (ret != Ok)
    {
        abort_with_report();
    }
}

void assert_not_null(void *p)
{
    if (p == NULL)
    {
        abort_with_report();
    }
}

int main()
{
    SpringErrno ret;

    SpringConfig *config = spring_config_default();
    assert_not_null(config);

    // Create a pipeline (dataflow definition in SpringQL).
    SpringPipeline *pipeline = spring_open(config);
    assert_not_null(pipeline);

    // Create a source stream. A stream is similar to a table in relational database.
    // A source stream is a stream to fetch stream data from a foreign source.
    //
    // `ROWTIME` is a keyword to declare that the column represents the event time of a row.
    ret = spring_command(
        pipeline,
        "CREATE SOURCE STREAM source_temperature_celsius ("
        "    ts TIMESTAMP NOT NULL ROWTIME,"
        "    temperature FLOAT NOT NULL"
        ");");
    assert_ok(ret);

    // Create a sink stream.
    // A sink stream is a final stream in a pipeline. A foreign sink fetches stream rows from it.
    ret = spring_command(
        pipeline,
        "CREATE SINK STREAM sink_temperature_fahrenheit ("
        "    ts TIMESTAMP NOT NULL ROWTIME,"
        "    temperature FLOAT NOT NULL"
        ");");
    assert_ok(ret);

    // Create a pump to convert Celsius to Fahrenheit.
    // A pump fetches stream rows from a stream, make some conversions to the rows, and emits them to another stream.
    ret = spring_command(
        pipeline,
        "CREATE PUMP c_to_f AS"
        "    INSERT INTO sink_temperature_fahrenheit (ts, temperature)"
        "    SELECT STREAM"
        "       source_temperature_celsius.ts,"
        "       32.0 + source_temperature_celsius.temperature * 1.8"
        "    FROM source_temperature_celsius;");
    assert_ok(ret);

    // Create a sink writer as an in-memory queue.
    // A sink writer fetches stream rows from a sink stream, and writes them to various foreign sinks.
    // Here we use an in-memory queue as the foreign sinks to easily get rows from our program written here.
    ret = spring_command(
        pipeline,
        "CREATE SINK WRITER queue_temperature_fahrenheit FOR sink_temperature_fahrenheit"
        "    TYPE IN_MEMORY_QUEUE OPTIONS ("
        "        NAME 'q'"
        "    );");
    assert_ok(ret);

    // Create a source reader as a TCP server (listening to tcp/9876).
    // A source reader fetches stream rows from a foreign source, and emits them to a source stream.
    //
    // Dataflow starts soon after this command creates the source reader instance.
    ret = spring_command(
        pipeline,
        "CREATE SOURCE READER tcp_trade FOR source_temperature_celsius"
        "    TYPE NET_SERVER OPTIONS ("
        "        PROTOCOL 'TCP',"
        "        PORT '9876'"
        "    );");
    assert_ok(ret);

    SpringSinkRow *row;
    while (1)
    {
        // Get a row,
        row = spring_pop(pipeline, "q");
        assert_not_null(row);

        // fetch columns,
        const int ts_len = 128;
        const char ts[ts_len];

        int r = spring_column_text(row, 0, (char *)ts, ts_len);
        assert(r == strlen(ts));

        float temperature_fahrenheit;
        ret = spring_column_float(row, 1, &temperature_fahrenheit);
        assert_ok(ret);

        // and print them.
        fprintf(stderr, "%s\t%f\n", ts, temperature_fahrenheit);

        // Free the row
        spring_sink_row_close(row);
    }

    // Free the remaining resources
    ret = spring_close(pipeline);
    assert_ok(ret);

    ret = spring_config_close(config);
    assert_ok(ret);

    return 0;
}
