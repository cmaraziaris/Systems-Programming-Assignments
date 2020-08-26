

// Search in the database for a patient record with id <rec_id>.
// If found, send a message over <write_fd> with the result (record).
void q_search_patient(char *rec_id, int write_fd, int buf_size);


// Send a message over <write_fd> with the total number of patients 
// that ENTER'ed in date range [entry_dt, exit_dt] with <disease> from <country>.
void q_disease_frequency(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size);


// Send a message over <write_fd> with the total number of patients 
// that ENTER'ed in date range [entry_dt, exit_dt] with <disease> from <country>.
// If <country> is NULL, send a message for every country handled by the worker.
void q_num_pat_admissions(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size, struct list *open_dirs);


// Send a message over <write_fd> with the total number of patients 
// that EXIT'ted in date range [entry_dt, exit_dt] with <disease> from <country>.
// If <country> is NULL, send a message for every country handled by the worker.
void q_num_pat_discharges(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size, struct list *open_dirs);


// Add the contents of the report <rep> to the database.
// Sets up data required for the `topk` query.
void q_add_report(char *rep);


// Answers the topk-AgeRanges query.
// Sends the result over <write_fd>.
void q_find_topk(char *msg, int write_fd, const int buf_size);