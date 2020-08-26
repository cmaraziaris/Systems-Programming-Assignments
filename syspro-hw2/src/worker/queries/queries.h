
// Search in the database for a patient record with id <rec_id>.
// If found, send a message to the parent with the result (record).
void q_search_patient(char *rec_id, int write_fd, int buf_size);

// Send a message to the parent with the total number of patients 
// that ENTER'ed in date range [entry_dt, exit_dt] with <disease> from <country>.
void q_disease_frequency(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size);

// Send a message to the parent with the total number of patients 
// that ENTER'ed in date range [entry_dt, exit_dt] with <disease> from <country>.
// If <country> is NULL, send a message for every country handled by the worker.
void q_num_pat_admissions(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size, struct list *open_dirs);

// Send a message to the parent with the total number of patients 
// that EXIT'ted in date range [entry_dt, exit_dt] with <disease> from <country>.
// If <country> is NULL, send a message for every country handled by the worker.
void q_num_pat_discharges(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size, struct list *open_dirs);
