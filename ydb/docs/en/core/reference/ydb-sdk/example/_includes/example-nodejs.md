# App in Node.js

This page contains a detailed description of the code of
[basic-example-v2-with-query-service](https://github.com/ydb-platform/ydb-nodejs-sdk/tree/master/examples/basic-example-v2-with-query-service) and 
[basic-example-v1-with-table-service](https://github.com/ydb-platform/ydb-nodejs-sdk/tree/master/examples/basic-example-v1-with-table-service) 
those are available as part of the {{ ydb-short-name }} [Node.js SDK](https://github.com/ydb-platform/ydb-nodejs-sdk).

{% include [init.md](steps/01_init.md) %}

App code snippet for driver initialization:

{% list tabs %}

- Using `connectionString`
    
    ```ts
    const authService = getCredentialsFromEnv();
    logger.debug('Driver initializing...');
    const driver = new Driver({connectionString, authService});
    const timeout = 10000;
    if (!await driver.ready(timeout)) {
        logger.fatal(`Driver has not become ready in ${timeout}ms!`);
        process.exit(1);
    }
    ```
    
- Using `endpoint` and `database`
    
    ```ts
    const authService = getCredentialsFromEnv();
    logger.debug('Driver initializing...');
    const driver = new Driver({endpoint, database, authService});
    const timeout = 10000;
    if (!await driver.ready(timeout)) {
        logger.fatal(`Driver has not become ready in ${timeout}ms!`);
        process.exit(1);
    }
    ```
    
{% endlist %}

App code snippet for creating a session:

{% list tabs %}

- Using Query Service
    
    ```ts
    const result = await driver.queryClient.do({
        ...
        fn: async (session) => {
            ...
        }
    });
    ```

- Using Table Service
    
    ```ts
    await driver.tableClient.withSession(async (session) => {
        ...
    });
    ```
    
{% endlist %}

{% include [create_table.md](steps/02_create_table.md) %}

{% list tabs %}

- Using Query Service
    
    ```ts
    async function createTables(driver: Driver, logger: Logger) {
        logger.info('Dropping old tables and create new ones...');
        await driver.queryClient.do({
            fn: async (session) => {
                await session.execute({
                    text: `
                        DROP TABLE IF EXISTS ${SERIES_TABLE};
                        DROP TABLE IF EXISTS ${EPISODES_TABLE};
                        DROP TABLE IF EXISTS ${SEASONS_TABLE};
    
                        CREATE TABLE ${SERIES_TABLE}
                        (
                            series_id    UInt64,
                            title        Utf8,
                            series_info  Utf8,
                            release_date DATE,
                            PRIMARY KEY (series_id)
                        );
    
                        CREATE TABLE ${SEASONS_TABLE}
                        (
                            series_id   UInt64,
                            season_id   UInt64,
                            title UTF8,
                            first_aired DATE,
                            last_aired DATE,
                            PRIMARY KEY (series_id, season_id)
                        );
    
                        CREATE TABLE ${EPISODES_TABLE}
                        (
                            series_id  UInt64,
                            season_id  UInt64,
                            episode_id UInt64,
                            title      UTf8,
                            air_date   DATE,
                            PRIMARY KEY (series_id, season_id, episode_id),
                            INDEX      episodes_index GLOBAL ASYNC ON (air_date)
                        );`,
                });
            },
        });
    }
    ```
    
- Using Table Service
    
    To create tables, use the `Session.CreateTable()` method:
    
    ```ts
    async function createTables(session: Session, logger: Logger) {
        logger.info('Creating tables...');
        await session.createTable(
            'series',
            new TableDescription()
                .withColumn(new Column(
                    'series_id',
                    Types.UINT64,  // not null column
                ))
                .withColumn(new Column(
                    'title',
                    Types.optional(Types.UTF8),
                ))
                .withColumn(new Column(
                    'series_info',
                    Types.optional(Types.UTF8),
                ))
                .withColumn(new Column(
                    'release_date',
                    Types.optional(Types.DATE),
                ))
                .withPrimaryKey('series_id')
        );
    
        await session.createTable(
            'seasons',
            new TableDescription()
                .withColumn(new Column(
                    'series_id',
                    Types.optional(Types.UINT64),
                ))
                .withColumn(new Column(
                    'season_id',
                    Types.optional(Types.UINT64),
                ))
                .withColumn(new Column(
                    'title',
                    Types.optional(Types.UTF8),
                ))
                .withColumn(new Column(
                    'first_aired',
                    Types.optional(Types.DATE),
                ))
                .withColumn(new Column(
                    'last_aired',
                    Types.optional(Types.DATE),
                ))
                .withPrimaryKeys('series_id', 'season_id')
        );
    
        await session.createTable(
            'episodes',
            new TableDescription()
                .withColumn(new Column(
                    'series_id',
                    Types.optional(Types.UINT64),
                ))
                .withColumn(new Column(
                    'season_id',
                    Types.optional(Types.UINT64),
                ))
                .withColumn(new Column(
                    'episode_id',
                    Types.optional(Types.UINT64),
                ))
                .withColumn(new Column(
                    'title',
                    Types.optional(Types.UTF8),
                ))
                .withColumn(new Column(
                    'air_date',
                    Types.optional(Types.DATE),
                ))
                .withPrimaryKeys('series_id', 'season_id', 'episode_id')
        );
    }
    ```
    
{% endlist %}

You can use the `TableSession.describeTable()` method to view information about the table structure and make sure that it was properly created:

```ts
async function describeTable(session: Session, tableName: string, logger: Logger) {
    const result = await session.describeTable(tableName);
    for (const column of result.columns) {
        logger.info(`Column name '${column.name}' has type ${JSON.stringify(column.type)}`);
    }
}

await describeTable(session, 'series', logger);
await describeTable(session, 'seasons', logger);
await describeTable(session, 'episodes', logger);
```

_Note:_ Query Service does not support functionality similar to `Session.describeTable()`.

{% include [steps/03_write_queries.md](steps/03_write_queries.md) %}

Code snippet for data insert/update:

{% list tabs %}

- Using Query Service
    
    ```ts
    async function upsertSimple(driver: Driver, logger: Logger): Promise<void> {
        logger.info('Making an upsert...');
        await driver.queryClient.do({
            fn: async (session) => {
                 await session.execute({
                     text: `
                        UPSERT INTO ${EPISODES_TABLE} (series_id, season_id, episode_id, title)
                        VALUES (2, 6, 1, "TBD");`,
               })
            }
        });
        logger.info('Upsert completed.')
    }
    ```
    
- Using Table Service
    
    ```ts
    async function upsertSimple(session: Session, logger: Logger): Promise<void> {
        const query = `
    ${SYNTAX_V1}
    UPSERT INTO episodes (series_id, season_id, episode_id, title) VALUES
    (2, 6, 1, "TBD");`;
        logger.info('Making an upsert...');
        await session.executeQuery(query);
        logger.info('Upsert completed');
    }
    ```
    
{% endlist %}

{% include [steps/04_query_processing.md](steps/04_query_processing.md) %}

{% list tabs %}

- Using Query Service (rowMode: RowType.Native)
    
    The `QuerySession.execute()` method is used to execute YQL queries.
    
    Depending on the rowMode parameter, the data can be retrieved in javascript form or as YDB structures.
    
    ```ts
    async function selectNativeSimple(driver: Driver, logger: Logger): Promise<void> {
        logger.info('Making a simple native select...');
        const result = await driver.queryClient.do({
            fn: async (session) => {
                const {resultSets} =
                    await session.execute({
                        // rowMode: RowType.Native, // Result set cols and rows returned as native javascript values. It's default behaviour
                        text: `
                            SELECT series_id,
                                   title,
                                   release_date
                            FROM ${SERIES_TABLE}
                            WHERE series_id = 1;`,
                    });
                const {value: resultSet1} = await resultSets.next();
                const rows: any[][] = []
                for await (const row of resultSet1.rows) rows.push(row);
                return {cols: resultSet1.columns, rows};
            }
        });
        logger.info(`selectNativeSimple cols: ${JSON.stringify(result.cols, null, 2)}`);
        logger.info(`selectNativeSimple rows: ${JSON.stringify(result.rows, null, 2)}`);
    }
    ```
    
- Using Query Service (rowMode: RowType.Ydb)

    The `QuerySession.execute()` method is used to execute YQL queries.
    
    Depending on the rowMode parameter, the data can be retrieved in javascript form or as YDB structures.
    
    ```ts
    async function selectTypedSimple(driver: Driver, logger: Logger): Promise<void> {
        logger.info('Making a simple typed select...');
        const result = await driver.queryClient.do({
            fn: async (session) => {
                const {resultSets} =
                    await session.execute({
                        rowMode: RowType.Ydb, // enables typedRows() on result sets
                        text: `
                            SELECT series_id,
                                   title,
                                   release_date
                            FROM ${SERIES_TABLE}
                            WHERE series_id = 1;`,
                    });
                const {value: resultSet1} = await resultSets.next();
                const rows: Series[] = [];
                // Note: resultSet1.rows will iterate YDB IValue structures
                for await (const row of resultSet1.typedRows(Series)) rows.push(row);
                return {cols: resultSet1.columns, rows};
            }
        });
        logger.info(`selectTypedSimple cols: ${JSON.stringify(result.cols, null, 2)}`);
        logger.info(`selectTypedSimple rows: ${JSON.stringify(result.rows, null, 2)}`);
    }
    ```
    
- Using Table Service
    
    To execute YQL queries, use the `Session.executeQuery()` method.
    
    ```ts
    async function selectSimple(session: Session, logger: Logger): Promise<void> {
        const query = `
    ${SYNTAX_V1}
    SELECT series_id,
           title,
           release_date
    FROM series
    WHERE series_id = 1;`;
        logger.info('Making a simple select...');
        const {resultSets} = await session.executeQuery(query);
        const result = Series.createNativeObjects(resultSets[0]);
        logger.info(`selectSimple result: ${JSON.stringify(result, null, 2)}`);
    }
    ```
    
{% endlist %}

{% include [param_queries.md](steps/06_param_queries.md) %}

Here's a code sample that shows how to use the `Session.executeQuery()` method with the queries and parameters
prepared by `Session.prepareQuery()`.

{% list tabs %}

- Using Query Service

    There is no explicit Prepared Query option in the Query Service.  YDB determines the necessity of using this mode by itself.
    
    ```ts
    async function selectWithParameters(driver: Driver, data: ThreeIds[], logger: Logger): Promise<void> {
         
        await driver.queryClient.do({
            fn: async (session) => {
                for (const [seriesId, seasonId, episodeId] of data) {
                    const episode = new Episode({seriesId, seasonId, episodeId, title: '', airDate: new Date()});
                    const {resultSets, opFinished} = await session.execute({
                        parameters: {
                            '$seriesId': episode.getTypedValue('seriesId'),
                            '$seasonId': episode.getTypedValue('seasonId'),
                            '$episodeId': episode.getTypedValue('episodeId')
                        },
                        text: `
                            SELECT title,
                                   air_date
                            FROM episodes
                            WHERE series_id = $seriesId
                              AND season_id = $seasonId
                              AND episode_id = $episodeId;`
                    });
                    const {value: resultSet} = await resultSets.next();
                    const {value: row} = await resultSet.rows.next();
                    await opFinished;
                    logger.info(`Select prepared query ${JSON.stringify(row, null, 2)}`);
                }
            }
        });
    }
    ```

- Using Table Service
    
    ```ts
    async function selectPrepared(session: Session, data: ThreeIds[], logger: Logger): Promise<void> {
        const query = `
        ${SYNTAX_V1}
        DECLARE $seriesId AS Uint64;
        DECLARE $seasonId AS Uint64;
        DECLARE $episodeId AS Uint64;
    
        SELECT title,
               air_date
        FROM episodes
        WHERE series_id = $seriesId AND season_id = $seasonId AND episode_id = $episodeId;`;
        async function select() {
            logger.info('Preparing query...');
            const preparedQuery = await session.prepareQuery(query);
            logger.info('Selecting prepared query...');
            for (const [seriesId, seasonId, episodeId] of data) {
                const episode = new Episode({seriesId, seasonId, episodeId, title: '', airDate: new Date()});
                const {resultSets} = await session.executeQuery(preparedQuery, {
                    '$seriesId': episode.getTypedValue('seriesId'),
                    '$seasonId': episode.getTypedValue('seasonId'),
                    '$episodeId': episode.getTypedValue('episodeId')
                });
                const result = Series.createNativeObjects(resultSets[0]);
                logger.info(`Select prepared query ${JSON.stringify(result, null, 2)}`);
            }
        }
        await withRetries(select);
    }
    ```
    
{% endlist %}

{% include [scan-query.md](steps/08_scan_query.md) %}

{% list tabs %}

- Using Query Service

    In Query Service, the `QuerySession.execute()` method is used to retrieve data by a stream.
    
    ```ts
    async function selectWithParametrs(driver: Driver, data: ThreeIds[], logger: Logger): Promise<void> {
        logger.info('Selecting prepared query...');
        await driver.queryClient.do({
            fn: async (session) => {
                for (const [seriesId, seasonId, episodeId] of data) {
                    const episode = new Episode({seriesId, seasonId, episodeId, title: '', airDate: new Date()});
    
                    // Note: In query service execute() there is no "prepared query" option.
                    //       This behaviour applied by YDB according to an internal rule
    
                    const {resultSets, opFinished} = await session.execute({
                        parameters: {
                            '$seriesId': episode.getTypedValue('seriesId'),
                            '$seasonId': episode.getTypedValue('seasonId'),
                            '$episodeId': episode.getTypedValue('episodeId')
                        },
                        text: `
                            SELECT title,
                                   air_date
                            FROM episodes
                            WHERE series_id = $seriesId
                              AND season_id = $seasonId
                              AND episode_id = $episodeId;`
                    });
                    const {value: resultSet} = await resultSets.next();
                    const {value: row} = await resultSet.rows.next();
                    await opFinished;
                    logger.info(`Select prepared query ${JSON.stringify(row, null, 2)}`);
                }
            }
        });
    }
    ```
    
- Using Table Service
    
    ```ts
    async function executeScanQueryWithParams(session: Session, logger: Logger): Promise<void> {
        const query = `
            ${SYNTAX_V1}        
            DECLARE $value AS Utf8;
    
            SELECT key
            FROM ${TABLE}
            WHERE value = $value;`;
    
        logger.info('Making a stream execute scan query...');
    
        const params = {
            '$value': TypedValues.utf8('odd'),
        };
    
        let count = 0;
        await session.streamExecuteScanQuery(query, (result) => {
            logger.info(`Stream scan query partial result #${++count}: ${formatPartialResult(result)}`);
        }, params);
    
        logger.info(`Stream scan query completed, partial result count: ${count}`);
    }
    ```

{% endlist %}

{% include [transaction-control.md](steps/10_transaction_control.md) %}

Here's a code sample that demonstrates how to explicitly use the `Session.beginTransaction()` and `Session.сommitTransaction()` calls to create and terminate a transaction:

{% list tabs %}

- Using Query Service do()
    
    ```ts
    async function explicitTcl(driver: Driver, ids: ThreeIds, logger: Logger) {
        logger.info('Running prepared query with explicit transaction control...');
        await driver.queryClient.do({
            fn: async (session) => {
                await session.beginTransaction({serializableReadWrite: {}});
                const [seriesId, seasonId, episodeId] = ids;
                const episode = new Episode({seriesId, seasonId, episodeId, title: '', airDate: new Date()});
                await session.execute({
                    parameters: {
                        '$seriesId': episode.getTypedValue('seriesId'),
                        '$seasonId': episode.getTypedValue('seasonId'),
                        '$episodeId': episode.getTypedValue('episodeId')
                    },
                    text: `
                        UPDATE episodes
                        SET air_date = CurrentUtcDate()
                        WHERE series_id = $seriesId
                          AND season_id = $seasonId
                          AND episode_id = $episodeId;`
                })
                const txId = session.txId;
                await session.commitTransaction();
                logger.info(`TxId ${txId} committed.`);
            }
        });
    }
    ```

- Using Query Service doTx()
    
    ```ts
    async function transactionPerWholeDo(driver: Driver, ids: ThreeIds, logger: Logger) {
        logger.info('Running query with one transaction per whole doTx()...');
        await driver.queryClient.doTx({
            txSettings: {serializableReadWrite: {}},
            fn: async (session) => {
                const [seriesId, seasonId, episodeId] = ids;
                const episode = new Episode({seriesId, seasonId, episodeId, title: '', airDate: new Date()});
                await session.execute({
                    parameters: {
                        '$seriesId': episode.getTypedValue('seriesId'),
                        '$seasonId': episode.getTypedValue('seasonId'),
                        '$episodeId': episode.getTypedValue('episodeId')
                    },
                    text: `
                        UPDATE episodes
                        SET air_date = CurrentUtcDate()
                        WHERE series_id = $seriesId
                          AND season_id = $seasonId
                          AND episode_id = $episodeId;`
                })
                logger.info(`TxId ${session.txId} will be committed by doTx().`);
            }
        });
    }
    ```

- Using Table Service
    
    ```ts
    async function explicitTcl(session: Session, ids: ThreeIds, logger: Logger) {
        const query = `
        ${SYNTAX_V1}
        DECLARE $seriesId AS Uint64;
        DECLARE $seasonId AS Uint64;
        DECLARE $episodeId AS Uint64;
    
        UPDATE episodes
        SET air_date = CurrentUtcDate()
        WHERE series_id = $seriesId AND season_id = $seasonId AND episode_id = $episodeId;`;
        async function update() {
            logger.info('Running prepared query with explicit transaction control...');
            const preparedQuery = await session.prepareQuery(query);
            const txMeta = await session.beginTransaction({serializableReadWrite: {}});
            const [seriesId, seasonId, episodeId] = ids;
            const episode = new Episode({seriesId, seasonId, episodeId, title: '', airDate: new Date()});
            const params = {
                '$seriesId': episode.getTypedValue('seriesId'),
                '$seasonId': episode.getTypedValue('seasonId'),
                '$episodeId': episode.getTypedValue('episodeId')
            };
            const txId = txMeta.id as string;
            logger.info(`Executing query with txId ${txId}.`);
            await session.executeQuery(preparedQuery, params, {txId});
            await session.commitTransaction({txId});
            logger.info(`TxId ${txId} committed.`);
        }
        await withRetries(update);
    }
    ```
    
{% endlist %}

{% include [error-handling.md](steps/50_error_handling.md) %}
