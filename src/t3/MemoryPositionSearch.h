/****************************************************************************
 *  In memory position search - find position in games without database index
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2014, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/

#ifndef MEMORY_POSITION_SEARCH_H
#define MEMORY_POSITION_SEARCH_H

#include <algorithm>
#include <vector>
#include <string>
#include "thc.h"
#include "ProgressBar.h"
#include "ListableGame.h"
#include "MemoryPositionSearchSide.h"
#include "PatternMatch.h"

// For standard algorithm, works for any game
struct MpsSlow
{
    int black_count_target;
    int black_pawn_count_target;
    int white_count_target;
    int white_pawn_count_target;
    uint64_t *slow_rank8_ptr;
    uint64_t *slow_rank7_ptr;
    uint64_t *slow_rank6_ptr;
    uint64_t *slow_rank5_ptr;
    uint64_t *slow_rank4_ptr;
    uint64_t *slow_rank3_ptr;
    uint64_t *slow_rank2_ptr;
    uint64_t *slow_rank1_ptr;
    uint64_t *slow_rank8_target_ptr;
    uint64_t *slow_rank7_target_ptr;
    uint64_t *slow_rank6_target_ptr;
    uint64_t *slow_rank5_target_ptr;
    uint64_t *slow_rank4_target_ptr;
    uint64_t *slow_rank3_target_ptr;
    uint64_t *slow_rank2_target_ptr;
    uint64_t *slow_rank1_target_ptr;
    uint64_t slow_white_home_mask;
    uint64_t slow_white_home_pawns;
    uint64_t slow_black_home_mask;
    uint64_t slow_black_home_pawns;
    char     slow_target_squares[64];
    uint64_t slow_rank8_mask;
    uint64_t slow_rank7_mask;
    uint64_t slow_rank6_mask;
    uint64_t slow_rank5_mask;
    uint64_t slow_rank4_mask;
    uint64_t slow_rank3_mask;
    uint64_t slow_rank2_mask;
    uint64_t slow_rank1_mask;
};

// This part is used for fast re-initiation at the start of each game
struct MpsSlowInit
{
    MpsSide sides[2];
    thc::ChessRules cr;
};

// For quick (or fast or accelerated) algorithm, use for games without promotions
struct MpsQuick
{
    int black_dark_bishop_target;
    int black_light_bishop_target;
    int black_rook_target;
    int black_queen_target;
    int black_knight_target;
    int black_pawn_target;
    int white_dark_bishop_target;
    int white_light_bishop_target;
    int white_rook_target;
    int white_queen_target;
    int white_knight_target;
    int white_pawn_target;
    uint64_t *rank8_ptr;
    uint64_t *rank7_ptr;
    uint64_t *rank6_ptr;
    uint64_t *rank5_ptr;
    uint64_t *rank4_ptr;
    uint64_t *rank3_ptr;
    uint64_t *rank2_ptr;
    uint64_t *rank1_ptr;
    const uint64_t *rank8_target_ptr;
    const uint64_t *rank7_target_ptr;
    const uint64_t *rank6_target_ptr;
    const uint64_t *rank5_target_ptr;
    const uint64_t *rank4_target_ptr;
    const uint64_t *rank3_target_ptr;
    const uint64_t *rank2_target_ptr;
    const uint64_t *rank1_target_ptr;
    uint64_t rank3_target;
    uint64_t rank4_target;
    uint64_t rank5_target;
    uint64_t rank6_target;
    uint64_t rank7_target;
    uint64_t rank8_target;
    uint64_t rank1_target;
    uint64_t rank2_target;
    char     target_squares[64];
};

// This part is used for fast re-initiation at the start of each game
struct MpsQuickInit
{
    MpsSide side_white;
    MpsSide side_black;
    char squares[64];
};

struct DoSearchFoundGame
{
    int idx;            // index into memory db
    int game_id;
    unsigned short offset_first;
    unsigned short offset_last;
};

class MemoryPositionSearch
{
public:
    MemoryPositionSearch()
    {
        Init();
        hash_initial = msi.cr.Hash64Calculate();
    }
    bool TryFastMode( MpsSide *side );
    bool SearchGameBase( std::string &moves_in );   // the original version
    bool SearchGameOptimisedNoPromotionAllowed( const char *moves_in, unsigned short &offset_first, unsigned short &offset_last  );    // much faster
    bool SearchGameSlowPromotionAllowed(  const std::string &moves_in, unsigned short &offset_first, unsigned short &offset_last  );          // semi fast
    bool PatternSearchGameOptimisedNoPromotionAllowed( const char *moves_in, unsigned short &offset_first, unsigned short &offset_last  );    // much faster
    bool PatternSearchGameSlowPromotionAllowed( PatternMatch &pm, const std::string &moves_in, unsigned short &offset_first, unsigned short &offset_last  );          // semi fast
    int  GetNbrGamesFound() { return games_found.size(); }
    std::vector< smart_ptr<ListableGame> > *search_source;
    std::vector< smart_ptr<ListableGame> >  &GetVectorSourceGames()   { return *search_source; }
    std::vector<DoSearchFoundGame>          &GetVectorGamesFound() { return games_found; }
    
    void Init()
    {
        in_memory_game_cache.clear();
        search_position_set=false;
        search_source = &in_memory_game_cache;
        thc::ChessPosition *cp = static_cast<thc::ChessPosition *>(&msi.cr);
        cp->Init();
        MpsSide      sides[2];
        sides[0].white=true;
        sides[0].fast_mode=false;
        sides[1].white=false;
        sides[1].fast_mode=false;
        TryFastMode( &sides[0]);
        TryFastMode( &sides[1]);
        mqi_init.side_white = sides[0];
        mqi_init.side_black = sides[1];
        msi.sides[0] = sides[0];
        msi.sides[1] = sides[0];
        memcpy( mqi_init.squares, "rnbqkdnrpppppppp................................PPPPPPPPRNDQKBNR", 64 );
        mq.rank8_ptr = reinterpret_cast<uint64_t*>(&mqi.squares[ 0]);
        mq.rank7_ptr = reinterpret_cast<uint64_t*>(&mqi.squares[ 8]);
        mq.rank6_ptr = reinterpret_cast<uint64_t*>(&mqi.squares[16]);
        mq.rank5_ptr = reinterpret_cast<uint64_t*>(&mqi.squares[24]);
        mq.rank4_ptr = reinterpret_cast<uint64_t*>(&mqi.squares[32]);
        mq.rank3_ptr = reinterpret_cast<uint64_t*>(&mqi.squares[40]);
        mq.rank2_ptr = reinterpret_cast<uint64_t*>(&mqi.squares[48]);
        mq.rank1_ptr = reinterpret_cast<uint64_t*>(&mqi.squares[56]);
        mq.rank8_target_ptr = reinterpret_cast<uint64_t*>(&mq.target_squares[ 0]);
        mq.rank7_target_ptr = reinterpret_cast<uint64_t*>(&mq.target_squares[ 8]);
        mq.rank6_target_ptr = reinterpret_cast<uint64_t*>(&mq.target_squares[16]);
        mq.rank5_target_ptr = reinterpret_cast<uint64_t*>(&mq.target_squares[24]);
        mq.rank4_target_ptr = reinterpret_cast<uint64_t*>(&mq.target_squares[32]);
        mq.rank3_target_ptr = reinterpret_cast<uint64_t*>(&mq.target_squares[40]);
        mq.rank2_target_ptr = reinterpret_cast<uint64_t*>(&mq.target_squares[48]);
        mq.rank1_target_ptr = reinterpret_cast<uint64_t*>(&mq.target_squares[56]);
        ms.slow_rank8_ptr = reinterpret_cast<uint64_t*>(&msi.cr.squares[ 0]);
        ms.slow_rank7_ptr = reinterpret_cast<uint64_t*>(&msi.cr.squares[ 8]);
        ms.slow_rank6_ptr = reinterpret_cast<uint64_t*>(&msi.cr.squares[16]);
        ms.slow_rank5_ptr = reinterpret_cast<uint64_t*>(&msi.cr.squares[24]);
        ms.slow_rank4_ptr = reinterpret_cast<uint64_t*>(&msi.cr.squares[32]);
        ms.slow_rank3_ptr = reinterpret_cast<uint64_t*>(&msi.cr.squares[40]);
        ms.slow_rank2_ptr = reinterpret_cast<uint64_t*>(&msi.cr.squares[48]);
        ms.slow_rank1_ptr = reinterpret_cast<uint64_t*>(&msi.cr.squares[56]);
        ms.slow_rank8_target_ptr = reinterpret_cast<uint64_t*>(&ms.slow_target_squares[ 0]);
        ms.slow_rank7_target_ptr = reinterpret_cast<uint64_t*>(&ms.slow_target_squares[ 8]);
        ms.slow_rank6_target_ptr = reinterpret_cast<uint64_t*>(&ms.slow_target_squares[16]);
        ms.slow_rank5_target_ptr = reinterpret_cast<uint64_t*>(&ms.slow_target_squares[24]);
        ms.slow_rank4_target_ptr = reinterpret_cast<uint64_t*>(&ms.slow_target_squares[32]);
        ms.slow_rank3_target_ptr = reinterpret_cast<uint64_t*>(&ms.slow_target_squares[40]);
        ms.slow_rank2_target_ptr = reinterpret_cast<uint64_t*>(&ms.slow_target_squares[48]);
        ms.slow_rank1_target_ptr = reinterpret_cast<uint64_t*>(&ms.slow_target_squares[56]);
    }
    int  DoSearch( const thc::ChessPosition &cp, uint64_t position_hash, ProgressBar *progress );
    int  DoSearch( const thc::ChessPosition &cp, uint64_t position_hash, ProgressBar *progress, std::vector< smart_ptr<ListableGame> > *source );
    int  DoPatternSearch( PatternMatch &pm, ProgressBar *progress );
    int  DoPatternSearch( PatternMatch &pm, ProgressBar *progress, std::vector< smart_ptr<ListableGame> > *source );
    bool IsThisSearchPosition( const thc::ChessPosition &cp )
        { return search_position_set && cp==search_position; }

public:
    std::vector< smart_ptr<ListableGame> > in_memory_game_cache;

private:
    thc::ChessPosition search_position;
    bool search_position_set;
    std::vector<DoSearchFoundGame> games_found;
    MpsSlow      ms;
    MpsSlowInit  msi;
    MpsQuick     mq;
    MpsQuickInit mqi_init;
    MpsQuickInit mqi;
    uint64_t white_home_mask;
    uint64_t white_home_pawns;
    uint64_t black_home_mask;
    uint64_t black_home_pawns;
    uint64_t hash_target;
    uint64_t hash_initial;
    void QuickGameInit()
    {
        mqi = mqi_init;
    }
    void SlowGameInit()
    {
        thc::ChessPosition *cp = static_cast<thc::ChessPosition *>(&msi.cr);
        cp->Init();
        msi.sides[0] = mqi_init.side_white;
        msi.sides[1] = mqi_init.side_black;
    }
    thc::Move UncompressSlowMode( char code );
    thc::Move UncompressFastMode( char code, MpsSide *side, MpsSide *other );
};

#endif // MEMORY_POSITION_SEARCH_H
