
// Returns the number of patients with <disease> that ENTER'ed in range [sdate1, sdate2].
// Patients originate from <country>, if specified (not NULL).
int disease_frequency(char *disease, char *sdate1, char *sdate2, char *country);


// Returns the number of patients with <disease> from country <country> that EXIT'ted in range [sdate1, sdate2].
int disease_exit_frequency(char *disease, char *sdate1, char *sdate2, char *country);