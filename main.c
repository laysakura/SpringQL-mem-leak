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

    SpringPipeline *pipeline = spring_open(config);
    assert_not_null(pipeline);

    ret = spring_command(
        pipeline,
        "CREATE SOURCE STREAM source_1 ("
        "    b BLOB NOT NULL"
        ");");
    assert_ok(ret);

    ret = spring_command(
        pipeline,
        "CREATE SINK STREAM sink_1 ("
        "    b BLOB NOT NULL"
        ");");
    assert_ok(ret);

    // Create a pump to convert Celsius to Fahrenheit.
    // A pump fetches stream rows from a stream, make some conversions to the rows, and emits them to another stream.
    ret = spring_command(
        pipeline,
        "CREATE PUMP c_to_f AS"
        "    INSERT INTO sink_1 (b)"
        "    SELECT STREAM"
        "       source_1.b"
        "    FROM source_1;");
    assert_ok(ret);

    // Create a sink writer as an in-memory queue.
    // A sink writer fetches stream rows from a sink stream, and writes them to various foreign sinks.
    // Here we use an in-memory queue as the foreign sinks to easily get rows from our program written here.
    ret = spring_command(
        pipeline,
        "CREATE SINK WRITER queue_sink FOR sink_1"
        "    TYPE IN_MEMORY_QUEUE OPTIONS ("
        "        NAME 'q_sink'"
        "    );");
    assert_ok(ret);

    // Create a source reader as a TCP server (listening to tcp/9876).
    // A source reader fetches stream rows from a foreign source, and emits them to a source stream.
    //
    // Dataflow starts soon after this command creates the source reader instance.
    ret = spring_command(
        pipeline,
        "CREATE SOURCE READER q_src FOR source_1"
        "    TYPE IN_MEMORY_QUEUE OPTIONS ("
        "        NAME 'q_src'"
        "    );");
    assert_ok(ret);

    const int n_rows = 10;

    // push rows
    for (int i = 0; i < n_rows; ++i)
    {
        SpringSourceRowBuilder *builder = spring_source_row_builder();

        int b = 0x12345678 + i;
        spring_source_row_add_column_blob(builder, "b", &b, sizeof(int));

        SpringSourceRow *row = spring_source_row_build(builder);
        spring_push(pipeline, "q_src", row);

        spring_source_row_close(row);
    }

    SpringSinkRow *row;
    for (int i = 0; i < n_rows; ++i)
    {
        // Get a row,
        row = spring_pop(pipeline, "q_sink");
        assert_not_null(row);

        // fetch columns,
        int b;
        int r = spring_column_blob(row, 0, &b, sizeof(int));
        assert(r == sizeof(int));

        // and print them.
        fprintf(stderr, "%x\n", b);

        // Free the row
        spring_sink_row_close(row);
    }

    fprintf(stderr, "waiting for more rows (never comes)...\n");
    row = spring_pop(pipeline, "q_sink");

    // Free the remaining resources
    ret = spring_close(pipeline);
    assert_ok(ret);

    ret = spring_config_close(config);
    assert_ok(ret);

    return 0;
}
