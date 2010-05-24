/* Linked list automaton */
/* Only used in prolog import */

struct fsm_state_l {
  int state_no; /* State number */
  int in;
  int out;
  int target;
  int final_state;
  int start_state;
  struct fsm_state_l *next_fsm_state;
};

struct state_array {
    struct fsm_state *transitions;
};

struct fsm_trans_list {
    short int in;
    short int out;
    int target;
    struct fsm_trans_list *next;
};

struct fsm_state_list {
    _Bool used;
    _Bool is_final;
    _Bool is_initial;
    short int num_trans;
    int state_number;
    struct fsm_trans_list *fsm_trans_list;
};

struct fsm_sigma_list {
    char *symbol;
};

struct fsm_sigma_hash {
    char *symbol;
    short int sym;
    struct fsm_sigma_hash *next;
};

struct fsm_construct_handle {
    struct fsm_state_list *fsm_state_list;
    int fsm_state_list_size;
    struct fsm_sigma_list *fsm_sigma_list;
    int fsm_sigma_list_size;
    struct fsm_sigma_hash *fsm_sigma_hash;
    int fsm_sigma_hash_size;
    int maxstate;
    int maxsigma;
    int numfinals;
    char *name;
};

/* Automaton functions operating on fsm_state */
int add_fsm_arc(struct fsm_state *fsm, int offset, int state_no, int in, int out, int target, int final_state, int start_state);

struct fsm_state_l *fsm_state_to_fsm_state_l(struct fsm_state *fsm);
void fsm_state_l_to_fsm_state(struct fsm *net, struct fsm_state_l *fsml, int arccount);
struct fsm_state_l *fsm_state_l_add_arc(struct fsm_state_l *fsm_state_l, int state_no, int in, int out, int target, int final_state, int start_state);
struct fsm_state *fsm_state_copy(struct fsm_state *fsm_state, int linecount);

/* Functions for constructing a FSM arc-by-arc */
/* At the end of the constructions, the flags are updated automatically */

/* Call fsm_state_init with the alphabet size to initialize the new machine */
struct fsm_state *fsm_state_init(int sigma_size);

/* Call set current state before calling fsm_state_add_arc */
void fsm_state_set_current_state(int state_no, int final_state, int start_state);

/* Add an arc */
void fsm_state_add_arc(int state_no, int in, int out, int target, int final_state, int start_state);

/* Call fsm_state_close() when done with arcs to a state */
void fsm_state_close();

/* Call this when done with entire FSM */
void fsm_state_end_state();

struct state_array *map_firstlines(struct fsm *net);


void fsm_count(struct fsm *net);

void fsm_sort_lines(struct fsm *net);
void fsm_update_flags(struct fsm *net, int det, int pru, int min, int eps, int loop, int compl_);

/* Rewrite-related functions */
struct fsm *rewrite_cp_to_fst(struct fsm *net, char *lower_symbol, char *zero_symbol);
struct fsm *rewrite_cp(struct fsm *U, struct fsm *L);

int sort_cmp(const void *a, const void *b);

int find_arccount(struct fsm_state *fsm);
int find_arccount_l(struct fsm_state_l *fsml);
int fsm_state_l_mark_final(struct fsm_state_l *fsml, int number);

/* Internal int stack */
int int_stack_isempty();
int int_stack_isfull();
void int_stack_clear();
int int_stack_find (int entry);
int int_stack_cmp (const void *a, const void *b);
void int_stack_sort();
void int_stack_push(int c);
int int_stack_pop();
int int_stack_status();
int int_stack_peek();
int int_stack_size();

/* Internal ptr stack */
int ptr_stack_isempty();
void ptr_stack_clear();
void *ptr_stack_pop();
int ptr_stack_isfull();
void ptr_stack_push(void *ptr);

/* Sigma functions */
int sigma_add (char *symbol, struct sigma *sigma);
int sigma_add_number(struct sigma *sigma, char *symbol, int number);
int sigma_add_special (int symbol, struct sigma *sigma);
struct sigma *sigma_remove(char *symbol, struct sigma *sigma);

int sigma_find (char *symbol, struct sigma *sigma);
int sigma_find_number (int number, struct sigma *sigma);
char *sigma_string(int number, struct sigma *sigma);
int sigma_sort (struct fsm *net);
void sigma_cleanup (struct fsm *net, int force);
struct sigma *sigma_create ();
int sigma_size(struct sigma *sigma);
int sigma_max(struct sigma *sigma);
struct fsm_sigma_list *sigma_to_list(struct sigma *sigma);

/* Debug */
void xprintf(char *string);

/* UTF8 */
unsigned char *utf8code16tostr(char *str);
int utf8skip(char *str);
int utf8strlen(char *str);
int ishexstr(char *str);
void decode_quoted(char *s);
void dequote_string(char *s);
char *remove_trailing(char *s, char c);
char *escape_string(char *string, char chr);

/* Flag-related */
int flag_check(char *sm);
char *flag_get_name(char *string);
char *flag_get_value(char *string);
int flag_get_type(char *string);

/* Misc */
char *trim(char *string);
char *chop(char *string);
void strip_newline(char *s);
char *streqrep(char *s, char *old, char *new_);
char *xxstrndup(const char *s, size_t n);
char *xxstrdup(const char *s);
void *xxmalloc(size_t size);
void *xxcalloc(size_t nmemb, size_t size);
void *xxmalloc_atomic(size_t size);
void *xxrealloc(void *ptr, size_t size);
void xxfree(void *ptr);
void garbage_cleanup(void);
int next_power_of_two(int v);
