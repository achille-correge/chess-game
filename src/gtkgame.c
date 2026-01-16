#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "types.h"
#include "gtkgame.h"
#include "chess_logic.h"
#include "communication.h"
#include "uci_handling.h"

static BoardState *board_s;
static PositionList *pos_l = NULL;
static int mode;
static char color;
static bool is_selected = false;
static Coords init_co;
GtkWidget *board_widgets[8][8];
GtkWidget *moves_label;

SubProcess sub_process;
char engine_color;
char moves_history[32000];

void on_activate(GtkApplication *app)
{
    GtkWidget *window = gtk_application_window_new(app);
    GtkWidget *start_button = gtk_button_new_with_label("Click to start");
    gtk_window_set_title(GTK_WINDOW(window), "Chess");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    gtk_window_set_child(GTK_WINDOW(window), start_button);
    g_signal_connect(start_button, "clicked", G_CALLBACK(menugtk), window);
    gtk_widget_set_visible(window, TRUE);
}

void to_menugtk(GtkWidget *widget, GtkWidget *window)
{
    gtk_window_set_child(GTK_WINDOW(window), NULL);
    menugtk(NULL, window);
}

void menugtk(GtkApplication *app, GtkWidget *window)
{
    GtkWidget *pvp_button = gtk_button_new_with_label("play pvp");
    GtkWidget *pva_button = gtk_button_new_with_label("play pva");
    GtkWidget *semi_free_button = gtk_button_new_with_label("play semi-free");
    GtkWidget *free_mode_button = gtk_button_new_with_label("play free mode");
    GtkWidget *settings_button = gtk_button_new_with_label("settings");
    // Create a new grid
    GtkWidget *grid = gtk_grid_new();
    // Set the grid spacing
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    // Add the grid to the window
    gtk_window_set_child(GTK_WINDOW(window), grid);
    // Attach the buttons to the grid
    gtk_grid_attach(GTK_GRID(grid), pvp_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pva_button, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), semi_free_button, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), free_mode_button, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), settings_button, 0, 4, 1, 1);
    // center the menu
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);
    // Connect the buttons to the callback functions
    g_signal_connect(pvp_button, "clicked", G_CALLBACK(pvp), window);
    g_signal_connect(pva_button, "clicked", G_CALLBACK(pva), window);
    g_signal_connect(semi_free_button, "clicked", G_CALLBACK(semi_free), window);
    g_signal_connect(free_mode_button, "clicked", G_CALLBACK(free_mode), window);
    g_signal_connect(settings_button, "clicked", G_CALLBACK(settings), NULL);
    // Show the window
    gtk_widget_set_visible(window, TRUE);
}

char *get_label_piece(Piece p)
{
    if (p.color == 'w')
    {
        switch (p.name)
        {
        case 'P':
            return "♙";
        case 'R':
            return "♖";
        case 'N':
            return "♘";
        case 'B':
            return "♗";
        case 'Q':
            return "♕";
        case 'K':
            return "♔";
        default:
            return " ";
        }
    }
    else
    {
        switch (p.name)
        {
        case 'P':
            return "♟";
        case 'R':
            return "♜";
        case 'N':
            return "♞";
        case 'B':
            return "♝";
        case 'Q':
            return "♛";
        case 'K':
            return "♚";
        default:
            return " ";
        }
    }
}

/*
char *get_label_piece(piece p)
{
    char *label_piece = malloc(2 * sizeof(char));
    label_piece[0] = p.name;
    if (p.color == 'b')
    {
        label_piece[0] = label_piece[0] + 32;
    }
    label_piece[1] = '\0';
    return label_piece;
}*/

void draw_board()
{
    Piece selected_piece = get_piece(board_s->board, init_co);
    char *label_piece;
    Coords dest_co;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            dest_co.x = i;
            dest_co.y = j;
            label_piece = get_label_piece(board_s->board[i][j]);
            gtk_label_set_text(GTK_LABEL(gtk_widget_get_first_child(GTK_WIDGET(board_widgets[7 - i][j]))), label_piece);
            if (can_move_heuristic(board_s, selected_piece, init_co, dest_co, true))
            {
                // set widget color to light/dark green
                if ((i + j) % 2 == 0)
                {
                    gtk_widget_set_name(board_widgets[7 - i][j], "darkgreen");
                }
                else
                {
                    gtk_widget_set_name(board_widgets[7 - i][j], "lightgreen");
                }
            }
            else if (init_co.x == i && init_co.y == j && is_selected && !is_empty_coords(init_co))
            {
                // set widget color to blue
                gtk_widget_set_name(board_widgets[7 - i][j], "blue");
            }
            else
            {
                // set widget color to the original color
                if ((i + j) % 2 == 0)
                {
                    gtk_widget_set_name(board_widgets[7 - i][j], "darkbrown");
                }
                else
                {
                    gtk_widget_set_name(board_widgets[7 - i][j], "lightbrown");
                }
            }
        }
    }

    // to the right of the board, display the moves history
    char moves_text[32000];
    snprintf(moves_text, sizeof(moves_text), "Moves history:\n%s", moves_history);
    gtk_label_set_text(GTK_LABEL(moves_label), moves_text);
}

void engine_play()
{
    // wait that the draw_board function is finished
    while (g_main_context_iteration(NULL, FALSE))
    {
        // Do nothing, just iterate
    }
    if (pos_l == NULL)
    {
        display_victory((color == 'w') ? 'b' : 'w', gtk_widget_get_ancestor(board_widgets[0][0], GTK_TYPE_WINDOW));
        stop_engine(sub_process);
    }
    pos_l = play_turn(sub_process.write_pipe, sub_process.read_pipe, moves_history, pos_l, 1000, 1000);
    *board_s = *pos_l->board_s;
    color = (color == 'w') ? 'b' : 'w';
    if (check_end(pos_l, color) == 1)
    {
        display_victory((color == 'w') ? 'b' : 'w', gtk_widget_get_ancestor(board_widgets[0][0], GTK_TYPE_WINDOW));
        stop_engine(sub_process);
    }
    else if (check_end(pos_l, color) == 0)
    {
        display_draw(gtk_widget_get_ancestor(board_widgets[0][0], GTK_TYPE_WINDOW));
        stop_engine(sub_process);
    }
    init_co = empty_coords();
    draw_board();

    // print the moves history
    // printf("Moves history: %s\n", moves_history);
    // from the moves history, get the last move played by the engine. (strrchr to find the last space)
    char *last_move_str = strrchr(moves_history, ' ');
    if (last_move_str == NULL)
    {
        last_move_str = moves_history;
    }
    else
    {
        last_move_str++;
    }
    Move last_move;
    last_move.init_co.y = last_move_str[0] - 'a';
    last_move.init_co.x = last_move_str[1] - '1';
    last_move.dest_co.y = last_move_str[2] - 'a';
    last_move.dest_co.x = last_move_str[3] - '1';

    // color the last move squares in blue
    gtk_widget_set_name(board_widgets[7 - last_move.init_co.x][last_move.init_co.y], "blue");
    gtk_widget_set_name(board_widgets[7 - last_move.dest_co.x][last_move.dest_co.y], "blue");
}

// on_square_clicked function
void on_square_clicked(GtkGestureClick *gesture, GtkButton *event, GtkWidget *eventbox)
{
    int left, top, width, height;
    // GtkWidget *label = gtk_widget_get_first_child(eventbox);
    GtkWidget *grid = gtk_widget_get_parent(eventbox);
    gtk_grid_query_child(GTK_GRID(grid), eventbox,
                         &left, &top, &width, &height);
    int row, column;
    row = top;
    column = left - 1;
    // printf("clicked on square %c%d\n", 'a' + column, 8 - row);
    Coords co;
    co.x = 7 - row;
    co.y = column;
    if (is_selected)
    {
        if (can_move_heuristic(board_s, board_s->board[init_co.x][init_co.y], init_co, co, true) || mode == 4)
        {
            Move move;
            move.init_co = init_co;
            move.dest_co = co;
            move.promotion = ' ';
            board_s = move_piece(board_s, move);
            is_selected = false;
            pos_l = save_position(board_s, pos_l);
            concatenate_moves(moves_history, move);
            if (mode < 3)
            {
                color = (color == 'w') ? 'b' : 'w';
            }
        }
        else if (!is_empty(board_s->board[co.x][co.y]) && (board_s->board[co.x][co.y].color == color || mode >= 3))
        {
            init_co = co;
            is_selected = true;
        }
        else
        {
            is_selected = false;
            init_co = empty_coords();
        }
    }
    else
    {
        if (!is_empty(board_s->board[co.x][co.y]) && (board_s->board[co.x][co.y].color == color || mode >= 3))
        {
            init_co = co;
            is_selected = true;
        }
    }
    draw_board();
    if (!is_selected)
    {
        init_co = empty_coords();
    }
    GtkWidget *window = gtk_widget_get_ancestor(eventbox, GTK_TYPE_WINDOW);
    char color1 = (color == 'w' || color == ' ') ? 'w' : 'b';
    char color2 = (color1 == 'w') ? 'b' : 'w';
    bool mate = is_mate(board_s, color1);
    if ((mate && !is_check(board_s, color1)) || board_s->fifty_move_rule >= 100 || threefold_repetition(board_s, pos_l, 0) || insufficient_material(board_s))
    {
        // printf("mate: %d, check: %d, fifty: %d, threefold: %d\n", mate, is_check(board_s, color), board_s->fifty_move_rule, threefold_repetition(board_s, pos_l, 0));
        if (insufficient_material(board_s))
        {
            printf("insufficient material\n");
        }
        else if (mate)
        {
            printf("mate\n");
        }
        else if (board_s->fifty_move_rule >= 100)
        {
            printf("fifty move rule\n");
        }
        else
        {
            print_position_list(pos_l);
            printf("threefold repetition\n");
        }
        display_draw(window);
        stop_engine(sub_process);
    }
    else if (is_check(board_s, color1) && mate)
    {
        display_victory(color2, window);
        stop_engine(sub_process);
    }
    else
    {
        mate = is_mate(board_s, color2);
        if ((mate && !is_check(board_s, color) && mode >= 3) || board_s->fifty_move_rule > 49 || threefold_repetition(board_s, pos_l, 0))
        {
            display_draw(window);
            stop_engine(sub_process);
        }
        else if (is_check(board_s, color2) && mate)
        {
            display_victory(color, window);
            stop_engine(sub_process);
        }
    }
    if (mode == 2 && color == engine_color)
    {
        engine_play();
    }
}

void init_chess_window(GtkApplication *app, GtkWidget *window)
{
    GtkWidget *eventbox, *menu_button;
    GtkWidget *label;
    GtkGesture *gesture;
    // Set the window default size
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    // Create the main grid
    GtkWidget *main_grid = gtk_grid_new();
    gtk_window_set_child(GTK_WINDOW(window), main_grid); // Add the main grid to the window
    gtk_grid_set_row_spacing(GTK_GRID(main_grid), 40);
    gtk_grid_set_column_spacing(GTK_GRID(main_grid), 40);
    // center the main grid
    gtk_widget_set_halign(main_grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(main_grid, GTK_ALIGN_CENTER);
    // Create the chess board grid
    GtkWidget *chess_grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(main_grid), chess_grid, 0, 0, 1, 1); // Attach the chess grid to the main grid
    // Create the menu grid
    GtkWidget *menu_grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(main_grid), menu_grid, 1, 0, 1, 1); // Attach the menu grid to the main grid
    gtk_grid_set_row_spacing(GTK_GRID(menu_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(menu_grid), 10);

    // Create a CSS provider and load the CSS data
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css_provider, "style/style.css");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(css_provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);

    for (int i = 0; i < 8; i++)
    {
        eventbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        char label_str[2] = {8 - i + '0', '\0'};
        label = gtk_label_new(label_str);
        gtk_widget_add_css_class(eventbox, "square");
        gtk_grid_attach(GTK_GRID(chess_grid), eventbox, 0, i, 1, 1);
        gtk_box_append(GTK_BOX(eventbox), label);
        gtk_widget_set_size_request(label, 70, 70);
        for (int j = 0; j < 8; j++)
        {
            gesture = gtk_gesture_click_new();
            eventbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            board_widgets[i][j] = eventbox;
            gtk_grid_attach(GTK_GRID(chess_grid), board_widgets[i][j], j + 1, i, 1, 1);
            gtk_widget_add_controller(eventbox, GTK_EVENT_CONTROLLER(gesture));
            label = gtk_label_new(" ");
            // set label size
            gtk_widget_set_size_request(label, 80, 80);
            gtk_box_append(GTK_BOX(eventbox), label);
            gtk_widget_add_css_class(board_widgets[i][j], "square");
            // set square color
            if ((i + j) % 2 == 0)
            {
                gtk_widget_set_name(board_widgets[i][j], "darkbrown");
            }
            else
            {
                gtk_widget_set_name(board_widgets[i][j], "lightbrown");
            }
            g_signal_connect(gesture, "pressed", G_CALLBACK(on_square_clicked), eventbox);
        }
    }
    for (int j = 0; j < 8; j++)
    {
        eventbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        char label_str[2] = {j + 'a', '\0'};
        label = gtk_label_new(label_str);
        gtk_widget_add_css_class(eventbox, "square");
        gtk_grid_attach(GTK_GRID(chess_grid), eventbox, j + 1, 8, 1, 1);
        gtk_box_append(GTK_BOX(eventbox), label);
        gtk_widget_set_size_request(label, 30, 30);
    }
    // Create a new button for the menu
    menu_button = gtk_button_new_with_label("Menu");
    // Attach the button to the grid
    gtk_grid_attach(GTK_GRID(menu_grid), menu_button, 0, 4, 1, 1);
    // make the btton a normal size
    gtk_widget_set_size_request(menu_button, 60, 30);
    // Connect the button to the callback function
    g_signal_connect(menu_button, "clicked", G_CALLBACK(to_menugtk), window);

    // Init the moves history label with scrolling
    moves_label = gtk_label_new("Moves history:\n");
    gtk_label_set_wrap(GTK_LABEL(moves_label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(moves_label), PANGO_WRAP_WORD);
    
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, 200, 200);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), moves_label);
    
    gtk_grid_attach(GTK_GRID(gtk_widget_get_parent(board_widgets[0][0])), scrolled_window, 9, 0, 1, 2);

    // Init the chess board state and positions list
    free(board_s);
    board_s = init_board();
    init_co = empty_coords();
    free_position_list(pos_l);
    pos_l = NULL;
    pos_l = save_position(board_s, pos_l);
    moves_history[0] = '\0';
    // Draw the board
    draw_board();
    // Show the window
    gtk_widget_set_visible(window, TRUE);
}

// launch the menu gtk window
void start()
{
    // Create a new application
    GtkApplication *app = gtk_application_new("com.example.GtkApplication",
                                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    // Run the application
    g_application_run(G_APPLICATION(app), 0, NULL);
}

void pvp(GtkApplication *app, GtkWidget *window)
{
    mode = 1;
    color = 'w';
    engine_color = ' ';
    sub_process = empty_process();
    init_chess_window(app, window);
}

void pva(GtkApplication *app, GtkWidget *window)
{
    mode = 2;
    color = 'w';
    engine_color = 'b';
    sub_process = gen_process();
    start_engine(sub_process);
    init_chess_window(app, window);
}

void semi_free(GtkApplication *app, GtkWidget *window)
{
    mode = 3;
    color = ' ';
    engine_color = ' ';
    sub_process = empty_process();
    init_chess_window(app, window);
}

void free_mode(GtkApplication *app, GtkWidget *window)
{
    mode = 4;
    color = ' ';
    engine_color = ' ';
    sub_process = empty_process();
    init_chess_window(app, window);
}

void settings(GtkApplication *app)
{
    printf("settings available soon\n");
}

// Structure to hold the promotion choice and window reference
typedef struct
{
    char choice;
    GtkWidget *window;
} PromotionData;

// Callback function for button clicks
void on_promotion_button_clicked(GtkButton *button, gpointer data)
{
    PromotionData *promotion_data = (PromotionData *)data;
    const gchar *button_label = gtk_button_get_label(button);

    // Set the choice based on the button label
    if (g_strcmp0(button_label, "Queen") == 0)
    {
        promotion_data->choice = 'Q';
    }
    else if (g_strcmp0(button_label, "Rook") == 0)
    {
        promotion_data->choice = 'R';
    }
    else if (g_strcmp0(button_label, "Bishop") == 0)
    {
        promotion_data->choice = 'B';
    }
    else if (g_strcmp0(button_label, "Knight") == 0)
    {
        promotion_data->choice = 'N';
    }

    // Close the window
    if (promotion_data->window != NULL)
    {
        gtk_window_destroy(GTK_WINDOW(promotion_data->window));
        promotion_data->window = NULL;
    }
}

char prompt_promotion(BoardState *board_state, Piece move_piece, Coords init_coords, Coords new_coords)
{
    // Create a new window
    GtkWidget *promotion_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(promotion_window), "Promote Pawn");
    gtk_window_set_default_size(GTK_WINDOW(promotion_window), 600, 400);

    // Create a grid to hold the buttons
    GtkWidget *grid = gtk_grid_new();
    gtk_window_set_child(GTK_WINDOW(promotion_window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 40);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 40);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);

    // Create buttons for promotion options
    GtkWidget *button_queen = gtk_button_new_with_label("Queen");
    GtkWidget *button_rook = gtk_button_new_with_label("Rook");
    GtkWidget *button_bishop = gtk_button_new_with_label("Bishop");
    GtkWidget *button_knight = gtk_button_new_with_label("Knight");

    // Add buttons to the grid
    gtk_grid_attach(GTK_GRID(grid), button_queen, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_rook, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_bishop, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_knight, 1, 1, 1, 1);

    // Allocate memory for the promotion data
    PromotionData *promotion_data = g_malloc(sizeof(PromotionData));
    promotion_data->choice = '\0';
    promotion_data->window = promotion_window;

    // Connect button click signals
    g_signal_connect(button_queen, "clicked", G_CALLBACK(on_promotion_button_clicked), promotion_data);
    g_signal_connect(button_rook, "clicked", G_CALLBACK(on_promotion_button_clicked), promotion_data);
    g_signal_connect(button_bishop, "clicked", G_CALLBACK(on_promotion_button_clicked), promotion_data);
    g_signal_connect(button_knight, "clicked", G_CALLBACK(on_promotion_button_clicked), promotion_data);

    // Show the window
    gtk_widget_set_visible(promotion_window, TRUE);

    // Run the GTK main loop until the window is closed
    GMainContext *context = g_main_context_default();
    while (promotion_window != NULL && gtk_widget_get_visible(promotion_window))
    {
        g_main_context_iteration(context, TRUE);
    }

    // Get the choice and free the data
    char promotion_choice = promotion_data->choice;
    g_free(promotion_data);

    return promotion_choice;
}

void display_draw(GtkWidget *window)
{
    GtkWidget *dialog = gtk_message_dialog_new((GtkWindow *)window, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Draw!");
    gtk_widget_set_visible(dialog, TRUE);
    // destroy the dialog
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_close), dialog);
}

void display_victory(char color, GtkWidget *window)
{
    printf("Checkmate!\n");
    GtkWidget *dialog = gtk_message_dialog_new((GtkWindow *)window, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Victory for %s!", (color == 'w') ? "white" : "black");
    gtk_widget_set_visible(dialog, TRUE);
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_close), dialog);
}