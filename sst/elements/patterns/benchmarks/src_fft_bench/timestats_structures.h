#ifndef _TIMESTATS_STRUCTURES_H_
#define _TIMESTATS_STRUCTURES_H_
/* data structure for returning timing statistics */
typedef struct {
  double avgcompute_time[5]; /* each element i corresponds to avg computation overhead */
                             /* spent in phase i across all ranks (in sec) */

  double mincompute_time[5]; /* each element i corresponds to minimum computation overhead */
                             /* spent in phase i across all ranks (in sec) */

  double maxcompute_time[5]; /* each element i corresponds to maximum computation overhead */
                             /* spent in phase i across all ranks (in sec) */

  double avgcomm_time[5];    /* each element i corresponds to avg communication overhead */
                             /* spent in phase i across all ranks (in sec) */

  double mincomm_time[5];    /* each element i corresponds to minimum communication overhead */
                             /* spent in phase i across all ranks (in sec) */

  double maxcomm_time[5];    /* each element i corresponds to maximum communication overhead */
                             /* spent in phase i across all ranks (in sec) */

  double avg_tot_time;       /* avg total FFT time across all ranks (in sec) */

  double max_tot_time;       /* minimum total FFT time across all ranks (in sec) */

  double min_tot_time;       /* maximum total FFT time across all ranks (in sec) */

  int status;                /* signal to caller whether the returned stats are valid. If -1 is */
                             /* is returned, an error has occurred and as such the results should */
                             /* be ignored. Otherwise, the results are valid. */
 } time_stat;

#endif /*  _TIMESTATS_STRUCTURES_H_ */
