/*--------------------------------------------------------------------------*\
 * dos2unix.c - Traiter les CR/LF dans les fichiers.
 *--------------------------------------------------------------------------
 * Copyright (c) 1990-2000 AbyreSoft. Written by BRy.
 *--------------------------------------------------------------------------
 * Creation   : 16/05/01
 * Evolutions : None
 * 22/05/01   : Ajout de l'option -u pour Unix vers Dos
 *--------------------------------------------------------------------------
 * Remarque : Ne pas oublier d'ouvrir le fichier de sortie en mode "b"
 * (untranslated mode) sinon chaque fois que l'on veut ecrire 0x0A dans le 
 * fichier fwrite ecrit en fait 0x0D 0x0xA.
 * \r = 0x0D = Carriage return
 * \n = 0x0A = New Line mais je crois qu'on dit aussi Line Feed (LF)
\*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>   /* _getche */
#include <io.h>      /* _finddata_t _findfirst ,_findnext, _findclose */

/*--------------------------------------------------------------------------*/

#define MAXCAR    64     /* Max de caracteres dans un nom de fichier */
#define MAXLISTE  100    /* Max de fichiers a traiter dans un liste  */
#define MAXCHAINE 128    /* Longueur max des chaines                 */

/*--------------------------------------------------------------------------*/

/* Boolean definition */
#define boolean unsigned char
#define false ((boolean)0)
#define true  (!false)

/*--------------------------------------------------------------------------*/

/* Debug functions */
#ifdef NO_DEBUG
#   include "pmTrace.h"
#   define  as_printf       pm_trace0
#else
#   define  as_printf	    printf
#   define  Dos2UnixMain    main
#endif

/*--------------------------------------------------------------------------*/

typedef enum
{
    E_UNKWON,
    E_DOS,
    E_UNIX,
    E_DOS_UNIX
}
t_TypeFile;

/*--------------------------------------------------------------------------*/

static void         DemandeParametres( void );
static t_TypeFile   DetermineTypeFile( FILE *fileIn );
static int          DosToUnix( FILE *fileIn, FILE *fileOut  );
static int          UnixToDos( FILE *fileIn, FILE *fileOut  );
static int          CompterCRLF( FILE *fileIn );

/*--------------------------------------------------------------------------*/

int Dos2UnixMain( int argc, char *argv[] )
{
    FILE       *file_in, *file_out1 = NULL;
    struct      _finddata_t file_infos;
    long        handle_file;
    int         nb_crlf = 0;
    int         nb_lf = 0;
    t_TypeFile  type_file = E_UNKWON;
    char        liste_files[MAXLISTE][MAXCAR];
    char        liste_fichiers[MAXLISTE][MAXCAR];
    boolean     demande_confirmation = false;
    boolean     comptage_seulement   = false;
    boolean     determine_type       = false;
    boolean     dos_to_unix          = true;
    int         i,j;

    /* Initialisations */
    for ( i = 0 ; i < MAXLISTE ; i++ )
    {
        liste_files[i][0] = '\x0';
        liste_fichiers[i][0] = '\x0';
    }

     /* Traiter les parametres */
    if ( argc < 2 )
    {
        DemandeParametres();
    }
    for ( i = 1, j = 0 ; i < argc; i++ ) /* argv[0] : nom du programme en cours */
    {                                    /* argc    : compteur des arguments    */
        if ( strcmp( argv[i], "?") == 0 ) 
        {
            DemandeParametres();
        } 
        else if ( strncmp( argv[i], "-c", 2) == 0 ) 
        {
            demande_confirmation = true;
        } 
        else if ( strncmp( argv[i], "-n", 2) == 0 ) 
        {
            comptage_seulement = true;
        } 
        else if ( strncmp( argv[i], "-d", 2) == 0 ) 
        {
            determine_type = true;
        } 
        else if ( strncmp( argv[i], "-u", 2) == 0 ) 
        {
            dos_to_unix = false;
        } 
        else 
        {
           /* Si ce n'est pas une option, c'est un fichier */
            strcpy_s( liste_files[j], argv[i] );
            j = j + 1;
        }
    }/* fin du for ( argc ) */

    /* Constituer la liste des fichiers a filtrer */
    if ( liste_files[0][0] != '\x0' ) /* la liste n'est pas vide */
    {
        as_printf("Liste des fichiers a traiter :\n");
    }
    i = 0;
    j = 0;
    while ( liste_files[i][0] != '\x0' ) /* fin de la liste */
    {
        handle_file = _findfirst( liste_files[i], &file_infos );
        if( handle_file == -1L )
        {
          as_printf("Pas de fichier %s dans le repertoire courant !\n", liste_files[i] );
          break;
        } 
        else 
        {
          do 
          {
             /* Les fichiers . et .. ne sont pas a traiter */
             if (    ( strcmp( file_infos.name, "." )     != 0 )
                  && ( strncmp( file_infos.name, "..", 2 ) != 0 ) 
                )
             {
                as_printf("  %s\n", file_infos.name );
                strcpy_s( liste_fichiers[j++], file_infos.name );
             }
          } 
          while ( _findnext( handle_file, &file_infos ) == 0 );

          _findclose( handle_file );
          i++;   
        }
    }
    if ( j == 0 )
    {
       as_printf("Pas de fichiers trouves.\n");
       exit( EXIT_SUCCESS );
    }

    /* Demander confirmation a l'utilisateur pour traiter les fichiers */
    if ( demande_confirmation )
    {
       printf("Voulez vous continuer <o/n> ? : ");
       if ( _getche() == 'n' )
       {
          exit( EXIT_SUCCESS );
       }
       printf("\n");
    }

    /* Effectuer le traitement des fichiers dans la liste */
    i = 0;
    while ( liste_fichiers[i][0] != '\x0' ) /* fin de la liste */
    {
        as_printf("Fichier %s : ", liste_fichiers[i] );
        errno_t err;

        /* Demander confirmation pour traiter ce fichier */
        if ( demande_confirmation )
        {
            printf("Voulez vous continuer <o/n> ? : ");
            if ( _getche() == 'n' )
            {
             exit( EXIT_SUCCESS );
            }
            printf("\n");
        }

        /* Ouvrir le fichier a traiter */
        err = fopen_s( &file_in, liste_fichiers[i], "rb" );
        if ( file_in == NULL )
        {
            as_printf("Impossible d'ouvrir le fichier %s\n", liste_fichiers[i] );
            exit( EXIT_FAILURE );
        }

        /* On ne fait que compter les CR/Lf */
        if ( comptage_seulement )
        {
            nb_crlf = CompterCRLF( file_in );
            if ( nb_crlf != 0 )
            {
                as_printf("%3d CR/LF\n", nb_crlf );
            }

            /* Passer au fichier suivant */
            fclose( file_in );
            i = i + 1;
            continue; //exit( EXIT_SUCCESS );
        }

        /* On ne fait que determiner le type du fichier */
        if ( determine_type )
        {
            type_file = DetermineTypeFile( file_in );
            switch ( type_file ) 
            {
                case E_UNKWON :
                    as_printf("%3d type UNKWON\n", nb_crlf );
                    break;
                case E_DOS :
                    as_printf("%3d type DOS\n", nb_crlf );
                    break;
                case E_UNIX :
                    as_printf("%3d type UNIX\n", nb_crlf );
                    break;
                case E_DOS_UNIX :
                    as_printf("%3d type DOS et UNIX\n", nb_crlf );
                    break;
            }

            /* Passer au fichier suivant */
            fclose( file_in );
            i = i + 1;
            continue; //exit( EXIT_SUCCESS );
        }

        /* Ouvrir le fichier temporaire 1 */
        err = fopen_s(&file_out1 , "temp1", "wb" );
        if ( file_out1 == NULL )
        {
            as_printf("Impossible d'ouvrir le fichier temporaire 1 !\n");
            exit( EXIT_FAILURE );
        }

        /* Effectuer le traitement */
        if ( dos_to_unix )
        {
            nb_crlf = DosToUnix( file_in, file_out1 );
        }
        else
        {
            nb_lf = UnixToDos( file_in, file_out1 );
        }

        /* Fermer les fichiers avant remove() et rename() */
        fclose( file_out1 );
        fclose( file_in );

        /* Y a-t-il des CR/LF dans ce fichier ? */
        if ( nb_crlf == 0 && nb_lf == 0 ) 
        {
             if ( dos_to_unix )
                 as_printf(" est propre de CR/LF.\n");
             else
                 as_printf(" est propre de LF.\n");

             /* S'il a ete cree, supprimer le fichier temporaire 1 */
             if ( file_out1 )
             {
                 if ( remove( "temp1" ) == -1 )
                 {
                    as_printf("Impossible de supprimer le fichier temp1 ! \n");
                    exit( EXIT_FAILURE );
                 }
             }
        }


        /* Si on a trouve quelque chose dans le fichier */
        if ( nb_crlf != 0 || nb_lf != 0 )
        {
            if ( ! comptage_seulement )
            {
                /* Supprimer le fichier de depart */
                if ( remove( liste_fichiers[i] ) == -1 )
                {
                    as_printf("Impossible de supprimer le fichier : %s !\n", liste_fichiers[i] );
                    exit( EXIT_FAILURE );
                }

                /* Rennomer le fichier temporaire 1 */
                if ( rename( "temp1", liste_fichiers[i] ) != 0 )
                {
                    as_printf("Renommage du fichier temporaire 1 impossible !\n");
                    exit( EXIT_FAILURE );
                }
            }

            /* Afficher les resultats */
            if ( nb_crlf != 0 )
            {
                as_printf("%3d CR/LF\n", nb_crlf );
            }
            if ( nb_lf != 0 )
            {
                as_printf("%3d LF\n", nb_lf );
            }
       }

       /* Passer au fichier suivant */
       i = i + 1;
    }

    return EXIT_SUCCESS;

}/* fin du main() */

/*--------------------------------------------------------------------------*\
 * Determine le type du fichier.
 * Si on ne trouve que des sequences 0x0D 0x0A c'est un fichier Windows.
 * Si on ne trouve que des sequences 0x0A c'est un fichier Unix.
 * fileIn  : fichier a traiter
 * retourne le type du fichier trouve
\*--------------------------------------------------------------------------*/
static t_TypeFile DetermineTypeFile( FILE *fileIn )
{
    t_TypeFile retour = E_UNKWON;
    int c;
    int nb_crlf = 0;

    while ( ( c = getc( fileIn ) ) != EOF )
    {
       if ( c == '\r' )
       {
          if ( ( c = getc( fileIn ) ) == '\n' )
          {
              if ( retour == E_UNIX )   retour = E_DOS_UNIX;
              if ( retour == E_UNKWON ) retour = E_DOS;
          }
          else
          {
             ungetc( c , fileIn );
          }
       } 
       else if ( c == '\n' )
       {
           if ( retour == E_DOS )    retour = E_DOS_UNIX;
           if ( retour == E_UNKWON ) retour = E_UNIX;
       }
    }

    return retour;
}

/*--------------------------------------------------------------------------*\
 * Transforme les 0x0D 0x0A des fichiers windows en 0x0A des fichiers Unix
 * fileIn  : fichier a traiter
 * fileOut : fichier de sortie
 * retourne le nombre de cr/lf trouves dans le fichier
\*--------------------------------------------------------------------------*/
static int DosToUnix( FILE *fileIn, FILE *fileOut  )
{
    int c;
    int nb_crlf = 0;

    while ( ( c = getc( fileIn ) ) != EOF )
    {
        if ( c == '\r' )
        {
          if ( ( c = getc( fileIn ) ) == '\n' )
          {
                nb_crlf++;
                putc( c, fileOut );
          }
          else
          {
                ungetc( c , fileIn );
          }
        }
        else
        {
            putc( c, fileOut );
        }
    }

    return nb_crlf;
}

/*--------------------------------------------------------------------------*\
 * Transforme les 0x0A des fichiers Unix en 0x0D 0x0A des fichiers windows.
 * fileIn  : fichier a traiter
 * fileOut : fichier de sortie
 * retourne le nombre de lf trouves dans le fichier
\*--------------------------------------------------------------------------*/
static int UnixToDos( FILE *fileIn, FILE *fileOut  )
{
    int c;
    static int r = '\r';
    int nb_lf = 0;
    
    while ( ( c = getc( fileIn ) ) != EOF )
    {
        if ( c == '\r' )
        {
            putc( c, fileOut );

            if ( ( c = getc( fileIn ) ) == '\n' )
            {
                putc( c, fileOut );
            }
            else
            {
                ungetc( c , fileIn );
            }
        }
        else if ( c == '\n' )
        {
            putc( r, fileOut );
            putc( c, fileOut );
            nb_lf++;
        }
        else
        {
            putc( c, fileOut );
        }
    }

    return nb_lf;

}

/*--------------------------------------------------------------------------*\
 * Compter les CR/LF dans un fichier
 * fileIn  : fichier a traiter
 * retourne le nombre de cr/lf trouves
\*--------------------------------------------------------------------------*/
static int CompterCRLF( FILE *fileIn )
{
    int c;
    int nb_crlf = 0;

    while ( ( c = getc( fileIn ) ) != EOF )
    {
       if ( c == '\r' )
       {
          if ( ( c = getc( fileIn ) ) == '\n' )
          {
             nb_crlf++;
          }
          else
          {
             ungetc( c , fileIn );
          }
       }
    }

    return nb_crlf;
}

/*--------------------------------------------------------------------------*/

void DemandeParametres( void )
{
    as_printf("\n");
    as_printf("Supra Couche DOS - BRy - Version 3.0\n\n");
    as_printf("Traiter les CR (LF) des fichiers Windows.\n\n");
    as_printf("Syntax:\n\n>dos2unix [-cndu]*.txt *.c *.* fichier.ext ...\n\n");
    as_printf("\t-c : demande de confirmation pour le traitement de chaque fichier\n");
    as_printf("\t-n : compter les cr/lf du fichier, ne pas les traiter\n");
    as_printf("\t-d : determiner le type du fichier\n");
    as_printf("\t-u : effectuer le traitement Unix->Dos\n");
    exit( EXIT_SUCCESS );

}/* fin de DemandeParametres() */
