/****************************************************************************
 * Custom dialog - Maintenance Dialog
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2014, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/
#define _CRT_SECURE_NO_DEPRECATE
#include "wx/wx.h"
#include "wx/valtext.h"
#include "wx/valgen.h"
#include "wx/file.h"
#include "wx/filename.h"
#include "wx/filepicker.h"
#include "DebugPrintf.h"
#include "Portability.h"
#include "Appdefs.h"
#include "PgnRead.h"
#include "ProgressBar.h"
#include "DbMaintenance.h"
#include "DbPrimitives.h"
#include "CreateDatabaseDialog.h"

// CreateDatabaseDialog type definition
IMPLEMENT_CLASS( CreateDatabaseDialog, wxDialog )

// CreateDatabaseDialog event table definition
BEGIN_EVENT_TABLE( CreateDatabaseDialog, wxDialog )

EVT_BUTTON( wxID_OK, /*ID_CREATE_DB_CREATE*/ CreateDatabaseDialog::OnOk )

EVT_BUTTON( wxID_HELP, CreateDatabaseDialog::OnHelpClick )
EVT_FILEPICKER_CHANGED( ID_CREATE_DB_PICKER_DB, CreateDatabaseDialog::OnDbFilePicked )
EVT_FILEPICKER_CHANGED( ID_CREATE_DB_PICKER1,   CreateDatabaseDialog::OnPgnFile1Picked )
EVT_FILEPICKER_CHANGED( ID_CREATE_DB_PICKER2,   CreateDatabaseDialog::OnPgnFile2Picked )
EVT_FILEPICKER_CHANGED( ID_CREATE_DB_PICKER3,   CreateDatabaseDialog::OnPgnFile3Picked )
END_EVENT_TABLE()

// CreateDatabaseDialog constructors
CreateDatabaseDialog::CreateDatabaseDialog()
{
    Init();
}

CreateDatabaseDialog::CreateDatabaseDialog(
                           wxWindow* parent,
                           wxWindowID id, const wxString& caption,
                           const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, caption, pos, size, style);
}

// Initialisation
void CreateDatabaseDialog::Init()
{
}

// Dialog create
bool CreateDatabaseDialog::Create( wxWindow* parent,
                          wxWindowID id, const wxString& caption,
                          const wxPoint& pos, const wxSize& size, long style )
{
    bool okay=true;
    
    // We have to set extra styles before creating the dialog
    SetExtraStyle( wxWS_EX_BLOCK_EVENTS|wxDIALOG_EX_CONTEXTHELP );
    if( !wxDialog::Create( parent, id, caption, pos, size, style ) )
        okay = false;
    else
    {
        
        CreateControls(id==ID_CREATE_DB_CREATE);
        SetDialogHelp();
        SetDialogValidators();
        
        // This fits the dialog to the minimum size dictated by the sizers
        GetSizer()->Fit(this);
        
        // This ensures that the dialog cannot be sized smaller than the minimum size
        GetSizer()->SetSizeHints(this);
        
        // Centre the dialog on the parent or (if none) screen
        Centre();
    }
    return okay;
}

// Control creation for CreateDatabaseDialog
void CreateDatabaseDialog::CreateControls( bool create )
{
    
    // A top-level sizer
    wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(top_sizer);
    
    // A second box sizer to give more space around the controls
    wxBoxSizer* box_sizer = new wxBoxSizer(wxVERTICAL);
    top_sizer->Add(box_sizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    
    // A friendly message
    wxStaticText* descr = new wxStaticText( this, wxID_STATIC,
           create?
           "To create a new database from scratch, name the new database and select\n"
           "one or more .pgn files with the games to go into the database.\n"
           :
           "To append to an existing database, select the database and \n"
           "one or more .pgn files with the games to go into the database.\n"
           , wxDefaultPosition, wxDefaultSize, 0 );
    box_sizer->Add(descr, 0, wxALIGN_LEFT|wxALL, 5);
    
    // Spacer
    box_sizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    
    // Label for file
    wxStaticText* db_file_label = new wxStaticText ( this, wxID_STATIC,
                                                 create ?
                                                    wxT("&Choose a new database file") :
                                                    wxT("&Choose an existing database file"),
                                                    wxDefaultPosition, wxDefaultSize, 0 );
    box_sizer->Add(db_file_label, 0, wxALIGN_LEFT|wxALL, 5);
    
    // File picker controls
    
    wxFilePickerCtrl *picker_db = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER_DB, db_filename,
                                                       create?
                                                       wxT("Select database to create") :
                                                       wxT("Select database to append to"),
                                                    "*.tarrasch_db", wxDefaultPosition, wxDefaultSize,
                                                       create?
                                                       wxFLP_USE_TEXTCTRL|wxFLP_SAVE :
                                                            wxFLP_USE_TEXTCTRL|wxFLP_FILE_MUST_EXIST
                                                       );
    box_sizer->Add(picker_db, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

    // Label for file
    wxStaticText* pgn_file_label = new wxStaticText ( this, wxID_STATIC,
                                                 wxT("&Select one or more .pgn files to add to the database"), wxDefaultPosition, wxDefaultSize, 0 );
    box_sizer->Add(pgn_file_label, 0, wxALIGN_LEFT|wxALL, 5);
    wxFilePickerCtrl *picker1 = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER1, pgn_filename1, wxT("Select 1st .pgn"),
                                                    "*.pgn", wxDefaultPosition, wxDefaultSize,
                                                    wxFLP_USE_TEXTCTRL|wxFLP_OPEN|wxFLP_FILE_MUST_EXIST ); //|wxFLP_CHANGE_DIR );
    box_sizer->Add(picker1, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

    wxFilePickerCtrl *picker2 = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER2, pgn_filename2, wxT("Select 2nd .pgn"),
                                                    "*.pgn", wxDefaultPosition, wxDefaultSize,
                                                    wxFLP_USE_TEXTCTRL|wxFLP_OPEN|wxFLP_FILE_MUST_EXIST ); //|wxFLP_CHANGE_DIR );
    box_sizer->Add(picker2, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

    wxFilePickerCtrl *picker3 = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER3, pgn_filename3, wxT("Select next .pgn"),
                                                    "*.pgn", wxDefaultPosition, wxDefaultSize,
                                                    wxFLP_USE_TEXTCTRL|wxFLP_OPEN|wxFLP_FILE_MUST_EXIST ); //|wxFLP_CHANGE_DIR );
    box_sizer->Add(picker3, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);
    
    // A dividing line before the database buttons
    wxStaticLine* line = new wxStaticLine ( this, wxID_STATIC,
                                           wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    box_sizer->Add(line, 0, wxGROW|wxALL, 5);
    
    // Temporary primitive database management functions
/*    wxBoxSizer* db_vert  = new wxBoxSizer(wxVERTICAL);
    box_sizer->Add( db_vert, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5);
    wxButton* button_cmd_5 = new wxButton( this, ID_CREATE_DB_CREATE, wxT("&Create the database"),
                                          wxDefaultPosition, wxDefaultSize, 0 );
    db_vert->Add( button_cmd_5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxButton* button_cmd_6 = new wxButton( this, ID_CREATE_DB_APPEND, wxT("&Append to the database"),
                                          wxDefaultPosition, wxDefaultSize, 0 );
    db_vert->Add( button_cmd_6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
  */
    
    // A dividing line before the OK and Cancel buttons
    wxStaticLine* line2 = new wxStaticLine ( this, wxID_STATIC,
                                           wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    box_sizer->Add(line2, 0, wxGROW|wxALL, 5);
    
    // A horizontal box sizer to contain Reset, OK, Cancel and Help
    wxBoxSizer* okCancelBox = new wxBoxSizer(wxHORIZONTAL);
    box_sizer->Add(okCancelBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 15);
  
   
    
    // The OK button
    wxButton* ok = new wxButton ( this, wxID_OK, wxT("&OK"),
                                 wxDefaultPosition, wxDefaultSize, 0 );
    okCancelBox->Add(ok, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    
    // The Cancel button
    wxButton* cancel = new wxButton ( this, wxID_CANCEL,
                                     wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    okCancelBox->Add(cancel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    
    // The Help button
    wxButton* help = new wxButton( this, wxID_HELP, wxT("&Help"),
                                  wxDefaultPosition, wxDefaultSize, 0 );
    okCancelBox->Add(help, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
}

// Set the validators for the dialog controls
void CreateDatabaseDialog::SetDialogValidators()
{
}

// Sets the help text for the dialog controls
void CreateDatabaseDialog::SetDialogHelp()
{
}

// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CREATE_DB_CREATE
void CreateDatabaseDialog::OnOk( wxCommandEvent& WXUNUSED(event) )
{
    bool ok=true;
    std::string files[6];
    int cnt=0;
    std::string error_msg;
    db_primitive_error_msg();   // clear error reporting mechanism
    
    // Collect files
    for( int i=1; i<=6; i++ )
    {
        bool exists = false;
        std::string filename;
        switch(i)
        {
            case 1: exists = wxFile::Exists(pgn_filename1);
                    filename = std::string(pgn_filename1.c_str());  break;
            case 2: exists = wxFile::Exists(pgn_filename2);
                    filename = std::string(pgn_filename2.c_str());  break;
            case 3: exists = wxFile::Exists(pgn_filename3);
                    filename = std::string(pgn_filename3.c_str());  break;
            case 4: exists = wxFile::Exists(pgn_filename4);
                    filename = std::string(pgn_filename4.c_str());  break;
            case 5: exists = wxFile::Exists(pgn_filename5);
                    filename = std::string(pgn_filename5.c_str());  break;
            case 6: exists = wxFile::Exists(pgn_filename6);
                    filename = std::string(pgn_filename6.c_str());  break;
        }
        if( exists )
        {
            files[cnt++] = filename;
        }
    }
    bool exists = wxFile::Exists(db_filename);
    std::string db_name = std::string( db_filename.c_str() );
    if( exists )
    {
        error_msg = "Tarrasch database file " + db_name + " already exists";
        ok = false;
    }
    else if( db_name.length() == 0 )
    {
        error_msg = "No Tarrasch database file specified";
        ok = false;
    }
    else if( cnt == 0 )
    {
        error_msg = "No usable pgn files specified";
        ok = false;
    }
    if( ok )
        ok = db_primitive_open( db_name.c_str() );
    if( ok )
        ok = db_primitive_transaction_begin();
    if( ok )
        ok = db_primitive_create_tables();
    db_primitive_count_games(); // to set game_id to zero
    for( int i=0; ok && i<cnt; i++ )
    {
        FILE *ifile = fopen( files[i].c_str(), "rt" );
        if( !ifile )
        {
            error_msg = "Cannot open ";
            error_msg += files[i];
            ok = false;
        }
        else
        {
            std::string title( "Creating database, step 2 of 4");
            std::string desc("Reading file #");
            char buf[80];
            sprintf( buf, "%d", i+1 );
            desc += buf;
            ProgressBar progress_bar( title, desc, ifile );
            PgnRead *pgn = new PgnRead('A',&progress_bar);
            bool aborted = pgn->Process(ifile);
            if( aborted )
            {
                error_msg = "cancel";
                ok = false;
            }
            delete pgn;
            fclose(ifile);
        }
    }
    if( ok )
        ok = db_primitive_flush();
    if( ok )
        ok = db_primitive_transaction_end();
    if( ok )
        ok = db_primitive_create_indexes();
    if( ok )
        ok = db_primitive_create_extra_indexes();
    if( ok )
        db_primitive_close();
    if( ok )
        AcceptAndClose();
    else
    {
        if( error_msg == "" )
            error_msg = db_primitive_error_msg();
        else if( error_msg == "cancel" )
            error_msg = "Database creation cancelled";
        wxMessageBox(error_msg.c_str());
        db_primitive_close();
        unlink(db_filename.c_str());
    }
}

void CreateDatabaseDialog::OnDbFilePicked( wxFileDirPickerEvent& event )
{
    wxString file = event.GetPath();
    db_filename = file;
}

void CreateDatabaseDialog::OnPgnFile1Picked( wxFileDirPickerEvent& event )
{
    wxString file = event.GetPath();
    pgn_filename1 = file;
}

void CreateDatabaseDialog::OnPgnFile2Picked( wxFileDirPickerEvent& event )
{
    wxString file = event.GetPath();
    pgn_filename2 = file;
}

void CreateDatabaseDialog::OnPgnFile3Picked( wxFileDirPickerEvent& event )
{
    wxString file = event.GetPath();
    pgn_filename3 = file;
}

void CreateDatabaseDialog::OnHelpClick( wxCommandEvent& WXUNUSED(event) )
{
    // Normally we would wish to display proper online help.
    // For this example, we're just using a message box.
    /*
     wxGetApp().GetHelpController().DisplaySection(wxT("Personal record dialog"));
     */
    
    wxString helpText =
    wxT("Add help text later.\n"); 
    wxMessageBox(helpText,
                 wxT("Maintenance dialog help"),
                 wxOK|wxICON_INFORMATION, this);
}
