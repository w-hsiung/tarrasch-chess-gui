/****************************************************************************
 *  Low level database maintenance functions
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2014, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include "thc.h"
#include "sqlite3.h"
#include "CompressMoves.h"
#include "DbPrimitives.h"
#include "DebugPrintf.h"
static void purge_bucket(int bucket_idx);
static void purge_buckets();
#define NBR_BUCKETS 4096
#define PURGE_QUOTA 10000

static void dump_game( FILE *f, int game_id );
void db_giri_game_hashes();


// A test program
int db_primitive_random_test_program()
{
    // Create an int variable for storing the return code for each call
    int retval;
    
    // A prepered statement for fetching tables
    sqlite3_stmt *stmt;
    
    // Create a handle for database connection, create a pointer to sqlite3
    sqlite3 *handle;
    
    // try to create the database. If it doesnt exist, it would be created
    // pass a pointer to the pointer to sqlite3, in short sqlite3**
    retval = sqlite3_open(DB_MAINTENANCE_FILE,&handle);

    // If connection failed, handle returns NULL
    if(retval)
    {
        cprintf("Database connection failed\n");
        return -1;
    }
    cprintf("Connection successful\n");
    
    // Create the SQL query for creating a table
    const char *create_table = "CREATE TABLE IF NOT EXISTS positions (game_id INTEGER, position_hash INTEGER)";
    
    // Execute the query for creating the table
    retval = sqlite3_exec(handle,create_table,0,0,0);
    if( retval )
    {
        cprintf("sqlite3_exec(CREATE) Failed\n");
        return -1;
    }
    
    
    // Delete previous data
    retval = sqlite3_exec( handle, "DELETE FROM positions",0,0,0);
    if( retval )
    {
        cprintf("sqlite3_exec(DELETE) Failed\n");
        return -1;
    }
    
    // Insert first row and second row
    char *errmsg;
    retval = sqlite3_exec( handle, "INSERT INTO positions VALUES(2,33334444)",0,0,&errmsg);
    if( retval )
    {
        cprintf("sqlite3_exec(INSERT 1) Failed %s\n", errmsg );
        return -1;
    }
    
    retval = sqlite3_exec( handle, "INSERT INTO positions VALUES(2,4444)",0,0,0);
    if( retval )
    {
        cprintf("sqlite3_exec(INSERT 2) Failed\n");
        return -1;
    }
    
 
    // select those rows from the table
    retval = sqlite3_prepare_v2( handle, "SELECT * from positions", -1, &stmt, 0 );
    if( retval )
    {
        cprintf("Selecting data from DB Failed\n");
        return -1;
    }
    
    // Read the number of rows fetched
    int cols = sqlite3_column_count(stmt);
    
    while(1)
    {
        // fetch a row's status
        retval = sqlite3_step(stmt);
        
        if(retval == SQLITE_ROW)
        {
            // SQLITE_ROW means fetched a row
            
            // sqlite3_column_text returns a const void* , typecast it to const char*
            for(int col=0 ; col<cols;col++)
            {
                int val = sqlite3_column_int(stmt,col);
                cprintf("%s = 0x%08x\t",sqlite3_column_name(stmt,col),val);
            }
            cprintf("\n");
        }
        else if(retval == SQLITE_DONE)
        {
            // All rows finished
            cprintf("All rows fetched\n");
            break;
        }
        else
        {
            // Some error encountered
            cprintf("Some error encountered\n");
            return -1;
        }
    }
    
    // Close the handle to free memory
    sqlite3_close(handle);
    return 0;
}

// Handle for database connection
static sqlite3 *handle;
static int game_id;

void db_primitive_open()
{
    cprintf( "db_primitive_open()\n" );
    
    // Try to create the database. If it doesnt exist, it would be created
    //  pass a pointer to the pointer to sqlite3, in short sqlite3**
    int retval = sqlite3_open(DB_MAINTENANCE_FILE,&handle);
    
    // If connection failed, handle returns NULL
    if(retval)
    {
        cprintf("DATABASE CONNECTION FAILED\n");
        return;
    }
    cprintf("Connection successful\n");
}

void db_primitive_create_tables()
{
    DebugPrintfTime turn_on_time_reporting;
    cprintf( "db_primitive_create_tables()\n" );
    
    // Create tables if not existing
    cprintf( "Create games table");
    int retval = sqlite3_exec(handle,"CREATE TABLE IF NOT EXISTS description (description TEXT, version INTEGER)",0,0,0);
    if( retval )
    {
        cprintf("sqlite3_exec(CREATE description) FAILED\n");
        return;
    }
    retval = sqlite3_exec(handle,"INSERT INTO description VALUES('Description of database goes here', 1 )",0,0,0);
    if( retval )
    {
        cprintf("sqlite3_exec(INSERT description) FAILED\n");
        return;
    }
    retval = sqlite3_exec(handle,"CREATE TABLE IF NOT EXISTS games (game_id INTEGER, game_hash INT8 UNIQUE, white TEXT, black TEXT, event TEXT, site TEXT, result TEXT, date TEXT, white_elo TEXT, black_elo TEXT, moves BLOB)",0,0,0);
    if( retval )
    {
        cprintf("sqlite3_exec(CREATE games) FAILED\n");
        return;
    }
    retval = sqlite3_exec(handle,"CREATE TABLE IF NOT EXISTS games_duplicates (game_id INTEGER, game_hash INT8, white TEXT, black TEXT, event TEXT, site TEXT, result TEXT, date TEXT, white_elo TEXT, black_elo TEXT, moves BLOB)",0,0,0);
    if( retval )
    {
        cprintf("sqlite3_exec(CREATE games_duplicates) FAILED\n");
        return;
    }
    cprintf( "Create positions tables");
    for( int i=0; i<NBR_BUCKETS; i++ )
    {
		if( i<20 || i%10 == 0)
			cprintf( "Creating bucket %d\n", i);
        char buf[200];
        sprintf( buf, "CREATE TABLE IF NOT EXISTS positions_%d (game_id INTEGER, position_hash INTEGER)", i );
        retval = sqlite3_exec(handle,buf,0,0,0);
        if( retval )
        {
            cprintf("sqlite3_exec(CREATE positions_%d) FAILED\n",i);
            return;
        }
    }
    cprintf( "Create positions tables end");
}

void db_primitive_delete_previous_data()
{
    int retval = sqlite3_exec( handle, "DELETE FROM games",0,0,0);
    if( retval )
    {
        cprintf("sqlite3_exec(DELETE games) Failed\n");
        return;
    }
    retval = sqlite3_exec( handle, "DELETE FROM positions",0,0,0);
    if( retval )
    {
        cprintf("sqlite3_exec(DELETE positions) Failed\n");
        return;
    }
}


void db_primitive_transaction_begin()
{
    char *errmsg=NULL;
    char buf[80];
    sprintf( buf, "BEGIN TRANSACTION" );
    int retval = sqlite3_exec( handle, buf,0,0,&errmsg);
    if( retval && errmsg )
        cprintf("sqlite3_exec(BEGIN TRANSACTION) FAILED [%s]\n", errmsg );
	else if (retval )
		cprintf("sqlite3_exec(BEGIN TRANSACTION) FAILED\n" );
}


void db_primitive_transaction_end()
{
    char *errmsg;
    char buf[80];
    purge_buckets();
    sprintf( buf, "COMMIT TRANSACTION" );
    int retval = sqlite3_exec( handle, buf,0,0,&errmsg);
    if( retval )
        cprintf("sqlite3_exec(COMMIT TRANSACTION) FAILED %s\n", errmsg );
}

void db_primitive_create_indexes()
{
    DebugPrintfTime turn_on_time_reporting;
    purge_buckets();
 /* moved to db_primitive_create_extra_indexes()
    for( int i=0; i<NBR_BUCKETS; i++ )
    {
        char buf[200];
        cprintf( "Create positions_%d index", i );
        sprintf( buf, "CREATE INDEX IF NOT EXISTS idx%d ON positions_%d(position_hash)",i,i);
        int retval = sqlite3_exec(handle,buf,0,0,0);
        if( retval )
        {
            cprintf( "sqlite3_exec(%s) FAILED\n", buf );
            return;
        }
    } */
    cprintf( "Create games index\n");
    int retval = sqlite3_exec(handle,"CREATE INDEX IF NOT EXISTS idx_games ON games(game_id)",0,0,0);
    cprintf( "Create games index end\n");
    if( retval )
    {
        cprintf("sqlite3_exec(CREATE INDEX games) FAILED\n");
    }
}

void db_primitive_create_extra_indexes()
{
    DebugPrintfTime turn_on_time_reporting;
    cprintf( "Create games(white) index\n");
    int retval = sqlite3_exec(handle,"CREATE INDEX IF NOT EXISTS idx_white ON games(white)",0,0,0);
    if( retval )
    {
        cprintf( "sqlite3_exec() FAILED 1\n" );
        return;
    }
    cprintf( "Create games(black) index\n");
    retval = sqlite3_exec(handle,"CREATE INDEX IF NOT EXISTS idx_black ON games(black)",0,0,0);
    if( retval )
    {
        cprintf( "sqlite3_exec() FAILED 1\n" );
        return;
    }
    //int retval = sqlite3_exec(handle,"DROP INDEX idx_white",0,0,0);
    cprintf( "Create games(white) index end\n");
    for( int i=0; i<NBR_BUCKETS; i++ )
    {
        char buf[200];
     /* no longer create idx%d in db_primitive_create_indexes();
        sprintf( buf, "DROP INDEX idx%d", i );
        cprintf( buf );
        retval = sqlite3_exec(handle,buf,0,0,0);
        if( retval )
        {
            cprintf( "sqlite3_exec() FAILED 2\n" );
            return;
        } */
        cprintf( "Create idx%d begin\n", i );
        sprintf( buf, "CREATE INDEX IF NOT EXISTS idx%d ON positions_%d(position_hash,game_id)",i,i);
        retval = sqlite3_exec(handle,buf,0,0,0);
        cprintf( "Create idx%d end\n", i );
        if( retval )
        {
            cprintf( "sqlite3_exec FAILED 3\n" );
            return;
        }
    }
}

/*
unsigned int levenshtein_distance(const std::string &s1, const std::string &s2)
{
	const size_t len1 = s1.size(), len2 = s2.size();
    std::vector<unsigned int> col(len2+1), prevCol(len2+1);
    
	for (unsigned int i = 0; i < prevCol.size(); i++)
		prevCol[i] = i;
	for (unsigned int i = 0; i < len1; i++)
    {
		col[0] = i+1;
		for (unsigned int j = 0; j < len2; j++)
        {
			col[j+1] = std::min( std::min(prevCol[1 + j] + 1, col[j] + 1),
								prevCol[j] + (s1[i]==s2[j] ? 0 : 1) );
        }
		col.swap(prevCol);
	}
	return prevCol[len2];
} 

static void t( const char *left, const char *right )
{
    unsigned int x = levenshtein_distance(left,right);
    cprintf( "[%s] [%s] %u\n", left, right, x );
} */



void db_primitive_speed_tests()
{
    DebugPrintfTime turn_on_time_reporting;
 /* t( "Smith", "Smith" );
    t( "Smith", "Simth" );
    t( "Smith", "Smithy" );
    t( "Smith", "Smiths" );
    t( "Smith", "Smmith" );
    t( "Smith", "Schmidt" );
    t( "Smith", "Smithson" );
    t( "Smith", "PSmith" );
    t( "Smith", "Brith" );
    t( "Smith", "Roberts" );
    t( "Smith", "Jones" ); */
    
    cprintf( "db_primitive_speed_tests()\n" );
    
    // Try to create the database. If it doesnt exist, it would be created
    //  pass a pointer to the pointer to sqlite3, in short sqlite3**
    int retval = sqlite3_open(DB_MAINTENANCE_FILE,&handle);
    
    // If connection failed, handle returns NULL
    if(retval)
    {
        cprintf("DATABASE CONNECTION FAILED\n");
        return;
    }
    cprintf("Connection successful\n");
    
    const char *magnus = "SELECT COUNT(*) from games, positions_2569 WHERE games.white = 'Carlsen, Magnus' "
                         "AND positions_2569.position_hash=44349918 AND games.game_id = positions_2569.game_id";
    const char *stamp =  "SELECT COUNT(*) from games, positions_3386 WHERE positions_3386.position_hash=2007903353 "
                         "AND games.game_id = positions_3386.game_id";
    const char *sicilian="SELECT COUNT(*) from games, positions_391 WHERE positions_391.position_hash=1592470315 "
                         "AND games.game_id = positions_391.game_id";

    char buf[1000];
    sqlite3_stmt *stmt;    // A prepared statement for fetching tables
    cprintf( "Database is %s\n", DB_MAINTENANCE_FILE );
    int results[5][3];
    time_t start_time;
    time ( &start_time );
    for( int i=0; i<5*3; i++ )
    {
        const char *query, *txt;
        switch(i%3)
        {
            case 0: query = magnus;     txt="Magnus"; break;
            case 1: query = stamp;      txt="Stamp"; break;
            case 2: query = sicilian;   txt="Sicilian"; break;
        }
        cprintf( "Test %d, %s; begin\n", i/3+1, txt );
        time_t begin = time(NULL);
        retval = sqlite3_prepare_v2( handle, query, -1, &stmt, 0 );
        if( retval )
        {
            cprintf("SELECTING DATA FROM DB FAILED 1\n");
        }
        retval = sqlite3_step(stmt);
        if( retval != SQLITE_ROW )
            cprintf("SELECTING DATA FROM DB FAILED 2\n");
        else
        {
            const char *val = (const char*)sqlite3_column_text(stmt,0);
            cprintf("Game count=%d\n", atoi(val) );
        }
        time_t end = time(NULL);
        int expired = (int)end-begin;
        cprintf( "Test %d, %s; end\n", i/3+1, txt );
        results[i/3][i%3] = expired;
        sqlite3_finalize(stmt);
    }
    
    for( int i=0; i<5*3; i++ )
    {
        int expired = results[i%5][i/5];
        if( i%5 == 0 )
        {
            const char *txt;
            switch(i/5)
            {
                case 0: txt="Magnus:";   break;
                case 1: txt="Stamp:";    break;
                case 2: txt="Sicilian:"; break;
            }
            cprintf( "%-10s", txt );
        }
        char buf[100];
        sprintf( buf, "%d:%02d", expired/60, expired%60 );
        cprintf( "%8s", buf );
        if( (i+1)%5 == 0 )
            cprintf( "\n" );
        else
            cprintf( "," );
    }
    time_t end_time;
    time ( &end_time );
    int total_elapsed = (end_time-start_time);
    cprintf( "Total elapsed: %d:%02d\n", total_elapsed/60, total_elapsed%60 );
    
    // Close the handle to free memory
    sqlite3_close(handle);
}


void db_giri_game_hashes()
{
    const char *a4[] =
    {
        "a4"
    };
    const char *giri_wangyue[] =
    {
        "c4", "Nf6", "Nf3", "c6", "g3", "g6", "Bg2", "Bg7", "O-O", "O-O", "d4", "d5", "Qb3", "Qb6", "Nc3", "Rd8",
        "Qa3", "dxc4", "Qxe7", "Re8", "Qa3", "Na6", "Qa4", "Qb4", "Qd1", "Bf5", "Nd2", "Be6", "e3", "Nd5", "Qc2",
        "Rad8", "a3", "Qe7", "Nxc4", "Nxc3", "Qxc3", "c5", "Ne5", "Bd5", "b4", "cxd4", "exd4", "Bxg2", "Kxg2", "Rxd4",
        "f4", "Rdd8", "Bb2", "Nb8", "Rae1", "Nd7", "Qc2", "Nxe5", "Bxe5", "Bxe5", "Rxe5", "Qf6", "Rfe1", "Rxe5", "Rxe5",
        "Qa6", "Qc3", "Qd6", "Qe3", "Qc6+", "Qe4", "Rd2+", "Kh3", "Qxe4", "Rxe4", "Kf8", "b5", "b6", "Ra4"
    };
    
    const char *giri_wangyue2[] =
    {
        "c4", "Nf6", "Nf3", "c6", "g3", "g6", "Bg2", "Bg7", "O-O", "O-O", "d4", "d5", "Qb3", "Qb6",
        "Nc3", "Rd8", "Qa3", "dxc4", "Qxe7", "Re8", "Qa3", "Na6", "Qa4", "Qb4", "Qd1", "Bf5",
        "Nd2", "Be6", "e3", "Nd5", "Qc2", "Rad8", "a3", "Qe7", "Nxc4", "Nxc3", "Qxc3", "c5",
        "Ne5", "Bd5", "b4", "cxd4", "exd4", "Bxg2", "Kxg2", "Rxd4", "f4", "Rdd8", "Bb2", "Nb8",
        "Rae1", "Nd7", "Qc2", "Nxe5", "Bxe5", "Bxe5", "Rxe5", "Qf6", "Rfe1", "Rxe5", "Rxe5", "Qa6",
        "Qc3", "Qd6", "Qe3", "Qc6+", "Qe4", "Rd2+", "Kh3", "Qxe4", "Rxe4", "Kf8", "b5", "b6",
        "Ra4"
    };
    
    /*
     [Event "World Cities Team GpF"]
     [Site "Al-Ain UAE"]
     [Date "2012.12.24"]
     [Round "3.1"]
     [White "Giri,A"]
     [Black "Wang Yue"]
     [Result "1/2-1/2"]
     [WhiteElo "2720"]
     [BlackElo "2696"]
     [ECO "D78"]
     
     1. c4 Nf6 2. Nf3 c6 3. g3 g6 4. Bg2 Bg7 5. O-O O-O 6. d4 d5 7. Qb3 Qb6 8.
     Nc3 Rd8 9. Qa3 dxc4 10. Qxe7 Re8 11. Qa3 Na6 12. Qa4 Qb4 13. Qd1 Bf5 14.
     Nd2 Be6 15. e3 Nd5 16. Qc2 Rad8 17. a3 Qe7 18. Nxc4 Nxc3 19. Qxc3 c5 20.
     Ne5 Bd5 21. b4 cxd4 22. exd4 Bxg2 23. Kxg2 Rxd4 24. f4 Rdd8 25. Bb2 Nb8 26.
     Rae1 Nd7 27. Qc2 Nxe5 28. Bxe5 Bxe5 29. Rxe5 Qf6 30. Rfe1 Rxe5 31. Rxe5 Qa6
     32. Qc3 Qd6 33. Qe3 Qc6+ 34. Qe4 Rd2+ 35. Kh3 Qxe4 36. Rxe4 Kf8 37. b5 b6
     38. Ra4 1/2-1/2
     
     
     
     
     */
    // 1.c4 Nf6 2.Nf3 c6 3.g3 g6 4.Bg2 Bg7 5.O-O O-O 6.d4 d5 7.Qb3 Qb6 8.Nc3 Rd8
    // 9.Qa3 dxc4 10.Qxe7 Re8 11.Qa3 Na6 12.Qa4 Qb4 13.Qd1 Bf5 14.Nd2 Be6 15.e3 Nd5 16.Qc2
    // Rad8 17.a3 Qe7 18.Nxc4 Nxc3
    // 19.Qxc3 c5 20.Ne5 Bd5 21.b4 cxd4 22.exd4 Bxg2 23.Kxg2 Rxd4
    // 24.f4 Rdd8 25.Bb2 Nb8 26.Rae1 Nd7 27.Qc2 Nxe5 28.Bxe5 Bxe5 29.Rxe5 Qf6 30.Rfe1 Rxe5 31.Rxe5
    // Qa6 32.Qc3 Qd6 33.Qe3 Qc6+ 34.Qe4 Rd2+ 35.Kh3 Qxe4 36.Rxe4 Kf8 37.b5 b6 38.Ra4 1/2-1/2
    
    
    for( int i=0; i<2; i++ )
    {
        const char **p;
        int nbr;
        thc::ChessRules cr;
        switch( i )
        {
            case 0: p = a4; nbr = 1; break;
            case 1: p = giri_wangyue2; nbr = sizeof(giri_wangyue2)/sizeof(giri_wangyue2[0]); break;
        }
        for( int j=0; j<nbr; j++ )
        {
            uint64_t hash64=cr.Hash64Calculate();
            thc::Move mv;
            const char *natural = p[j];
            bool ok = mv.NaturalIn( &cr, natural );
            if( !ok )
            {
                cprintf( "Whoops!\n" );
                break;
            }
            hash64 = cr.Hash64Update( hash64, mv );
            cr.PlayMove(mv);
            uint64_t bucket32 = (hash64>>32) & 0xfff;
            int bucket = (int)bucket32;
            uint64_t hash32 = hash64 & 0xffffffff;
            int hash = (int)hash32;
            //cprintf( "Move=%4s, bucket=%03x, hash=%08x, hash64=%lx\n", natural, bucket, hash, hash64 );
            cprintf( "Move=%4s, bucket=%d, hash=%d, hash64=%0lx\n", natural, bucket, hash, hash64 );
            //cprintf( "Move=%s, hash64=%lx\n", natural, hash64 );
        }
    }
}


void db_test_after_1a4()
{
    cprintf( "db_test_after_1a4()\n" );
    
    // Try to create the database. If it doesnt exist, it would be created
    //  pass a pointer to the pointer to sqlite3, in short sqlite3**
    int retval = sqlite3_open(DB_MAINTENANCE_FILE,&handle);
    
    // If connection failed, handle returns NULL
    if(retval)
    {
        cprintf("DATABASE CONNECTION FAILED\n");
        return;
    }
    cprintf("Connection successful\n");
    
    char buf[1000];
    sqlite3_stmt *stmt;    // A prepared statement for fetching tables
    cprintf( "Database is %s\n", DB_MAINTENANCE_FILE );
    //const char *query = "SELECT games.game_id from games JOIN positions_969 ON games.game_id = positions_969.game_id WHERE positions_969.position_hash=-1850675132 ORDER BY games.rowid DESC LIMIT 0,100";
    const char *query = "SELECT games.game_id from games JOIN positions_969 ON games.game_id = positions_969.game_id WHERE positions_969.position_hash=-1850675132 ORDER BY games.rowid";
    //const char *query = "SELECT game_id FROM positions_969 WHERE position_hash=-1850675132";

    retval = sqlite3_prepare_v2( handle, query, -1, &stmt, 0 );
    if( retval )
    {
        cprintf("SELECTING DATA FROM DB FAILED 1\n");
    }
    for(;;)
    {
        retval = sqlite3_step(stmt);
        if( retval != SQLITE_ROW )
            break;
        else
        {
            const char *val = (const char*)sqlite3_column_text(stmt,0);
            cprintf("game_id=%d\n", atoi(val) );
            //dump_game( stdout, atoi(val) );
            //cprintf("\n");
        }
    }
    sqlite3_finalize(stmt);
    
    // Close the handle to free memory
    sqlite3_close(handle);
}

void db_test_dump_selected_games()
{
    cprintf( "db_test_dump_selected_games()\n" );
    
    // Try to create the database. If it doesnt exist, it would be created
    //  pass a pointer to the pointer to sqlite3, in short sqlite3**
    int retval = sqlite3_open(DB_MAINTENANCE_FILE,&handle);
    
    // If connection failed, handle returns NULL
    if(retval)
    {
        cprintf("DATABASE CONNECTION FAILED\n");
        return;
    }
    cprintf("Connection successful\n");

#define PROBE 2516563
    struct { int n; const char *desc; } game_list[] =
    {
        {0,          "start"    },
        {1,          "start+1"  },
        {2,          "start+2"  },
        {3,          "start+3"  },
        {PROBE,      "probe"    },
        {PROBE+1,    "probe+1"  },
        {PROBE+2,    "probe+2"  },
        {PROBE+3,    "probe+3"  }
     /* {0,       "first game, start of giant part 1" },
        {2516563, "start of giant part 2" },
        {3495200, "start of twic no overlap" },
        {4201480, "start of twic extras" },
        {4227389, "before mystery game" },
        {4227390, "mystery game" },
        {4227391, "after mystery game" },
        {4358865, "last game"  } */
    };
    for( int i=0; i<sizeof(game_list)/sizeof(game_list[0]); i++ )
    {
        int n = game_list[i].n;
        const char *desc = game_list[i].desc;
        cprintf( "Showing %s game %d\n", desc, n );
        dump_game(stdout,n);
    }
    
    // Close the handle to free memory
    sqlite3_close(handle);
}




void db_primitive_close()
{
    purge_buckets();

    // Close the handle to free memory
    sqlite3_close(handle);
}


// get one game
static void dump_game( FILE *f, int game_id )
{
    // Get white player
    char buf[100];
    sqlite3_stmt *stmt;    // A prepared statement for fetching tables
    int retval;
    sprintf( buf, "SELECT white from games WHERE game_id=%d", game_id );
    retval = sqlite3_prepare_v2( handle, buf, -1, &stmt, 0 );
    if( retval )
    {
        cprintf("SELECTING DATA FROM DB FAILED\n");
    }
    retval = sqlite3_step(stmt);
    if(retval == SQLITE_ROW)
    {
        const char *val = (const char*)sqlite3_column_text(stmt,0);
        fprintf(f,"White:%s\n",val);
    }

    // Get black player
    sprintf( buf, "SELECT black from games WHERE game_id=%d", game_id );
    retval = sqlite3_prepare_v2( handle, buf, -1, &stmt, 0 );
    if( retval )
    {
        cprintf("SELECTING DATA FROM DB FAILED\n");
    }
    retval = sqlite3_step(stmt);
    if(retval == SQLITE_ROW)
    {
        const char *val = (const char*)sqlite3_column_text(stmt,0);
        fprintf(f,"Black:%s\n",val);
    }
    
    // Get result
    sprintf( buf, "SELECT result from games WHERE game_id=%d", game_id );
    retval = sqlite3_prepare_v2( handle, buf, -1, &stmt, 0 );
    if( retval )
    {
        cprintf("SELECTING DATA FROM DB FAILED\n");
    }
    retval = sqlite3_step(stmt);
    if(retval == SQLITE_ROW)
    {
        const char *val = (const char*)sqlite3_column_text(stmt,0);
        fprintf(f,"Result:%s\n",val);
    }
    
    // Get moves
    sprintf( buf, "SELECT moves from games WHERE game_id=%d", game_id );
    retval = sqlite3_prepare_v2( handle, buf, -1, &stmt, 0 );
    if( retval )
    {
        cprintf("SELECTING DATA FROM DB FAILED\n");
    }
    retval = sqlite3_step(stmt);
    if(retval == SQLITE_ROW)
    {
        int len = sqlite3_column_bytes(stmt,0);
        fprintf(f,"Move len = %d\n",len);
        const char *blob = (const char*)sqlite3_column_blob(stmt,0);
        fprintf(f,"Moves:\n");
        CompressMoves press;
        std::string compressed(blob,blob+len);
        std::vector<thc::Move> unpacked = press.Uncompress(compressed);
        thc::ChessRules cr;
        for( int i=0; i<len; i++ )
        {
            thc::Move mv=unpacked[i];
            std::string s = mv.NaturalOut(&cr);
            fprintf(f," %s",s.c_str());
            cr.PlayMove(mv);
        }
        fprintf(f,"\n");
    }
    
    /*
     *pnBlob = sqlite3_column_bytes(pStmt, 0);
     *pzBlob = (unsigned char *)malloc(*pnBlob);
     memcpy(*pzBlob, sqlite3_column_blob(pStmt, 0), *pnBlob);
    
    thc::ChessRules cr;
    for( int i=0; i<moves.size(); i++ )
    {
        int move = moves[i];
        thc::Move mv;
        memcpy(&mv,&move,sizeof(thc::Move));
        std::string s = mv.NaturalOut(&cr);
        cr.PlayMove(mv);
        fprintf(f," %s",s.c_str());
    }
    fprintf(f,"\n");
     */
    
}

// A real database test
void db_primitive_show_games( bool connect )
{
    DebugPrintfTime turn_on_time_reporting;
    cprintf( "db_primitive_show_games()\n" );
    int retval;
    if( connect )
    {
        // Try to create the database. If it doesnt exist, it would be created
        //  pass a pointer to the pointer to sqlite3, in short sqlite3**
        retval = sqlite3_open(DB_MAINTENANCE_FILE,&handle);
        
        // If connection failed, handle returns NULL
        if(retval)
        {
            cprintf("DATABASE CONNECTION FAILED\n");
            return;
        }
        cprintf("Connection successful\n");
        
        
    }
    
    thc::ChessRules cr;
    //cr.Forsyth("r1bqk2r/ppp1bppp/2n1pn2/3p4/Q1PP4/P3PN2/1P1N1PPP/R1B1KB1R b KQkq - 0 7");   // Bogo indian position after 8 moves or so 46 games
    cr.Forsyth("rnbqk2r/pppp1ppp/4pn2/8/1bPP4/2N5/PP2PPPP/R1BQKBNR w KQkq - 2 4");          // Nimzo-Indian start position 24000 games
    uint32_t hash = cr.HashCalculate();
    

    // select matching rows from the table
    char buf[100];
    sqlite3_stmt *stmt;    // A prepared statement for fetching tables
    sprintf( buf, "SELECT * from positions WHERE position_hash=%d", hash );
    retval = sqlite3_prepare_v2( handle, buf, -1, &stmt, 0 );
    if( retval )
    {
        cprintf("SELECTING DATA FROM DB FAILED\n");
    }
    
    // Read the number of rows fetched
    int cols = sqlite3_column_count(stmt);
    std::vector<int> games;

    cprintf( "Fetching games begin");
    while(1)
    {
        // fetch a row's status
        retval = sqlite3_step(stmt);
        
        if(retval == SQLITE_ROW)
        {
            // SQLITE_ROW means fetched a row
            
            // sqlite3_column_text returns a const void* , typecast it to const char*
            for(int col=0 ; col<cols;col++)
            {
                const char *val = (const char*)sqlite3_column_text(stmt,col);
                //cprintf("%s:%s\t",sqlite3_column_name(stmt,col),val);
                if( col == 0 )
                {
                    int game_id = atoi(val);
                    games.push_back(game_id);
                }
            }
            //cprintf("\n");
        }
        else if(retval == SQLITE_DONE)
        {
            // All rows finished
            break;
        }
        else
        {
            // Some error encountered
            cprintf("SOME ERROR ENCOUNTERED\n");
        }
    }
    cprintf("Fetching games end");

    FILE *pgn_out = fopen("/Users/billforster/Documents/games.txt","wt");
    cprintf("Dumping games begin");
    if( pgn_out )
    {
        for( int i=0; i<games.size(); i++ )
        {
            char buf[100];
            if( i%100 == 0 || i==games.size()-1 )
            {
                cprintf( "Game %d",i+1);
            }
            dump_game( pgn_out, games[i] );
        }
        fclose(pgn_out);
    }
    cprintf("Dumping games end");
    
    
    // Close the handle to free memory
    sqlite3_close(handle);
}



int db_primitive_count_games()
{
    int game_count=0;
    
    // select matching rows from the table
    char buf[100];
    sqlite3_stmt *stmt;    // A prepared statement for fetching tables
    sprintf( buf, "SELECT COUNT(*) from games" );
    int retval = sqlite3_prepare_v2( handle, buf, -1, &stmt, 0 );
    if( retval )
    {
        cprintf("SELECTING DATA FROM DB FAILED\n");
        return 0;
    }

    // Read the number of rows fetched
    int cols = sqlite3_column_count(stmt);

    cprintf( "Get games count begin\n");
    while(1)
    {
        // fetch a row's status
        retval = sqlite3_step(stmt);
        
        if(retval == SQLITE_ROW)
        {
            // SQLITE_ROW means fetched a row
            
            // sqlite3_column_text returns a const void* , typecast it to const char*
            for( int col=0; col<cols; col++ )
            {
                const char *val = (const char*)sqlite3_column_text(stmt,col);
                //cprintf("%s:%s\t",sqlite3_column_name(gbl_stmt,col),val);
                if( col == 0 )
                {
                    //int game_id = atoi(val);
                    //game_ids.push_back(game_id);
                    game_count = atoi(val);
                    cprintf( "Game count = %d\n", game_count );
                }
            }
        }
        else if( retval == SQLITE_DONE )
        {
            // All rows finished
            sqlite3_finalize(stmt);
            break;
        }
        else
        {
            // Some error encountered
            cprintf("SOME ERROR ENCOUNTERED\n");
        }
    }
    cprintf("Get games count end - %d games\n", game_count );
    game_id = game_count;
    return game_count;
}


static bool sanitise( const char *in, char *out, int out_siz )
{
    bool hasAlpha=false;
    int len=0;
    while( *in && len<out_siz-3 )
    {
        char c = *in++;
        if( !isascii(c) )
            *out = '_';
        else if( c!=' ' && c!='.' && c!=',' && !isalnum(c) )
            *out = '_';
        else
        {
            *out = c;
            if( !hasAlpha && isalpha(c) )
                hasAlpha = true;
        }
        out++;
        len++;
    }
    *out = '\0';
    return hasAlpha;
}


std::vector<std::pair<int,int>> buckets[NBR_BUCKETS];
static void purge_buckets()
{
    for( int i=0; i<NBR_BUCKETS; i++ )
    {
        purge_bucket(i);
    }
}

static void purge_bucket( int bucket_idx )
{
    char *errmsg;
    char insert_buf[2000];
    std::vector<std::pair<int,int>> *bucket = &buckets[bucket_idx];
    int count = bucket->size();
    if( count > 0 )
    {
        std::sort( bucket->begin(), bucket->end() );
        char buf[100];
        //cprintf( "purge bucket %d, %d items", bucket_idx, count );
        for( int j=0; j<count; j++ )
        {
            std::pair<int,int> duo = (*bucket)[j];
            sprintf( insert_buf, "INSERT INTO positions_%d VALUES(%d,%d)", bucket_idx, duo.second, duo.first );
            if( duo.second==4227390 && bucket_idx==0x3c9 && duo.second==-1850675132 /*0x91b0f044*/ )
            {
                cprintf( "*** TRIGGER b) *** Giri game wrongly linked to position after 1.a4\n" );
            }
            int retval = sqlite3_exec( handle, insert_buf,0,0,&errmsg);
            if( retval )
            {
                cprintf("sqlite3_exec(INSERT 3) FAILED %s\n", errmsg );
                return;
            }
        }
        bucket->clear();
    }
}


// Return true if this game already in database
bool db_primitive_check_for_duplicate( uint64_t game_hash, const char *white, const char *black, const char *result, const char *blob_moves )
{
    for( int table_idx=0; table_idx<2; table_idx++ )
    {
        // select matching rows from the table
        char buf[200];
        sqlite3_stmt *stmt;    // A prepared statement for fetching tables
        sprintf( buf, "SELECT white, black, result, moves from %s WHERE game_hash=%lld", table_idx==0?"games":"games_duplicates", game_hash );
        const char *errmsg;
        int retval = sqlite3_prepare_v2( handle, buf, -1, &stmt, &errmsg );
        if( retval )
        {
            cprintf("DB FAIL db_primitive_check_for_duplicate() 1 %s (%s)\n", errmsg, buf );
            return false;
        }
        
        // Read the game info
        bool white_match=false;
        bool black_match=false;
        bool result_match=false;
        bool moves_match=false;
        int cols = sqlite3_column_count(stmt);
        for(;;)  // Read one row from games, potentially many rows in games_duplicates
        {
            retval = sqlite3_step(stmt);
            if( retval == SQLITE_ROW )
            {
                for( int col=0; col<cols; col++ )
                {
                    if( col == 0 )
                    {
                        const char *val = (const char*)sqlite3_column_text(stmt,col);
                        if( val )
                            white_match = (0 == strcmp(white,val));
                    }
                    else if( col == 1 )
                    {
                        const char *val = (const char*)sqlite3_column_text(stmt,col);
                        if( val )
                            black_match = (0 == strcmp(black,val));
                    }
                    else if( col == 2 )
                    {
                        const char *val = (const char*)sqlite3_column_text(stmt,col);
                        if( val )
                            result_match = (0 == strcmp(result,val));
                    }
                    else if( col == 3 )
                    {
                        int len = sqlite3_column_bytes(stmt,col);
                        //fprintf(f,"Move len = %d\n",len);
                        //if( len > 10 )
                        //    cprintf( "A real game....\n" );
                        const char *blob = (const char*)sqlite3_column_blob(stmt,col);
                        if( blob )
                            moves_match = (0 == memcmp(blob_moves,blob,len));
                    }
                }
            }
            else if( retval == SQLITE_DONE )
            {
                // All rows finished
                sqlite3_finalize(stmt);
                stmt = NULL;
                if( white_match && black_match && result_match && moves_match )
                {
                    cprintf( "Duplicate game %s-%s %s found\n", white,black,result);
                    return true;    // duplicate found
                }
                break;
            }
            else
            {
                // Some error encountered
                cprintf("DB FAIL db_primitive_check_for_duplicate() 2\n");
                return false;
            }
        }
    }
    return false;   // return false unless we explicitly DID find a duplicate
}

void db_primitive_insert_game( const char *white, const char *black, const char *event, const char *site, const char *result,
                                    const char *date, const char *white_elo, const char *black_elo,
                                    int nbr_moves, thc::Move *moves, uint64_t *hashes  )
{
    //cprintf( "db_primitive_gameover(%s,%s)\n", white, black );
    //uint32_t *move_ptr = (uint32_t *)moves;
    char *errmsg;
    char insert_buf[2000];
    char blob_txt_buf[4000];    // about 500 moves each
    char blob_buf[2000];
    char white_buf[200];
    char black_buf[200];
    char event_buf[200];
    char site_buf[200];
    if( nbr_moves < 3 )    // skip 'games' with zero, one or two moves
        return;
    if( white_elo && black_elo )
    {
        int elo_w = atoi(white_elo);
        int elo_b = atoi(black_elo);
        if( 0<elo_w && elo_w<2000 && 0<elo_b && elo_b<2000 )
            return;   // if any elo information, need at least one good player
    }
    /* if( date && strlen(date)>=4 )
    {
        char buf[5];
        memcpy(buf,date,4);
        buf[4] = '\0';
        int year = atoi(buf);
        if( year >= 1980 )
        {
            if( white_elo && strlen(white_elo)>=4  && black_elo && strlen(black_elo)>=4 )
            {
                int elo_w = atoi(white_elo);
                int elo_b = atoi(black_elo);
                if( elo_w<2000 && elo_b<2000 )
                    return;
            }
        }                                    
    }  */
    bool alphaWhite = sanitise( white,  white_buf, sizeof(white_buf) );
    bool alphaBlack = sanitise( black,  black_buf, sizeof(black_buf) );
    if( !alphaWhite || !alphaBlack )    // skip games with no names
        return;
    sanitise( event,  event_buf, sizeof(event_buf) );
    sanitise( site, site_buf,  sizeof(site_buf) );
    CompressMoves press;
    uint64_t hash = press.cr.Hash64Calculate();
    uint64_t counter=0, game_hash=hash;
    char *put_txt = blob_txt_buf;
    char *put = blob_buf;
    for( int i=0; i<nbr_moves && put_txt<blob_txt_buf+sizeof(blob_txt_buf)-10; i++ )
    {
        thc::Move mv = moves[i];
        char c = press.CompressMove(mv);
        *put++ = c;
        char hi = (c>>4)&0x0f;
        *put_txt++ = (hi>=10 ? hi-10+'A' : hi+'0');
        char lo = c&0x0f;
        *put_txt++ = (lo>=10 ? lo-10+'A' : lo+'0');
        hash = press.cr.Hash64Update( hash, mv );
        game_hash = game_hash ^ hash ^ counter;
        counter++;
    }
    *put_txt = '\0';
    *put = '\0';
    sprintf( insert_buf, "INSERT INTO games VALUES(%d,%lld,'%s','%s','%s','%s','%s','%s','%s','%s',X'%s')",
                        game_id, game_hash, white_buf, black_buf, event_buf, site_buf, result,
                        date, white_elo, black_elo, blob_txt_buf );
    int retval = sqlite3_exec( handle, insert_buf,0,0,&errmsg);
    if( retval )
    {
        char buf[200];
        strncpy( buf, errmsg, sizeof(buf)-1 );
        buf[ sizeof(buf)-1 ] = '\0';
        char *p = buf;
        while( *p )
        {
            if( isalpha(*p) && isupper(*p) )
                *p = tolower(*p);
            p++;
        }
        if( !strstr(buf,"unique") )
        {
            cprintf("DB_FAIL db_primitive_insert_game() 1 %s (%s)\n", errmsg, insert_buf );
            return;
        }
        //cprintf("Non unique game hash %s (%s)\n", errmsg, insert_buf );

        // Check whether a duplicate of existing game
        bool is_duplicate = db_primitive_check_for_duplicate( game_hash, white_buf, black_buf, result, blob_buf );
        if( is_duplicate )
        {
            return;  // ZOMBIE bug fix - return here, don't add bogus moves and increment game count
        }
        // If not, put it into the database
        else
        {
            sprintf( insert_buf, "INSERT INTO games VALUES(%d,NULL,'%s','%s','%s','%s','%s','%s','%s','%s',X'%s')",
                    game_id, white_buf, black_buf, event_buf, site_buf, result,
                    date, white_elo, black_elo, blob_txt_buf );
            int retval = sqlite3_exec( handle, insert_buf,0,0,&errmsg);
            if( retval )
            {
                cprintf("DB_FAIL db_primitive_insert_game() 2 %s (%s)\n", errmsg, insert_buf );
                return;
            }
            sprintf( insert_buf, "INSERT INTO games_duplicates VALUES(%d,%lld,'%s','%s','%s','%s','%s','%s','%s','%s',X'%s')",
                    game_id, game_hash, white_buf, black_buf, event_buf, site_buf, result,
                    date, white_elo, black_elo, blob_txt_buf );
            retval = sqlite3_exec( handle, insert_buf,0,0,&errmsg);
            if( retval )
            {
                cprintf("DB_FAIL db_primitive_insert_game() 3 %s (%s)\n", errmsg, insert_buf );
                return;
            }
        }
    }
    for( int i=0; i<nbr_moves; i++ )
    {
        uint64_t hash64 = *hashes++;
        int hash32 = (int)(hash64);
        int table_nbr = ((int)(hash64>>32))&(NBR_BUCKETS-1);
        std::vector<std::pair<int,int>> *bucket = &buckets[table_nbr];
        std::pair<int,int> duo(hash32,game_id);
        bucket->push_back(duo);
        if( game_id==4227390 && table_nbr==0x3c9 && hash32==-1850675132 /*0x91b0f044*/ )
        {
            cprintf( "*** TRIGGER a) *** Giri game wrongly linked to position after 1.a4\n");
        }
        int count = bucket->size();
        if( count >= PURGE_QUOTA )
            purge_bucket(table_nbr);
    }
    game_id++;
}



