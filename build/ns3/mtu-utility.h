#ifndef MTU_UTILITY_H
#define MTU_UTILITY_H

#define CDF_TABLE_ENTRY 32

namespace ns3
{
struct cdf_entry
{
    double value;
    double cdf;
};

/*cdf distribution*/
struct cdf_table
{
    struct cdf_entry *entries;
    int num_entry;  /* number of entries in CDF table */
    int max_entry;  /* maximum number of entries in CDF table */
    double min_cdf; /* minimum value of CDF (default 0) */
    double max_cdf; /* maximum value of CDF (default 1) */
};

class MtuUtility
{
private:
    /* data */
public:
    /* initialize a CDF distribution */
    static void init_cdf(struct cdf_table *table);

    /* free resources of a CDF distribution */
    static void free_cdf(struct cdf_table *table);

    /* get CDF distribution from a given file */
    static void load_cdf(struct cdf_table *table, const char *file_name);

    /* print CDF distribution information */
    static void print_cdf(struct cdf_table *table);

    /* get average value of CDF distribution */
    static double avg_cdf(struct cdf_table *table);

    /* Generate a random value based on CDF distribution */
    static double gen_random_cdf(struct cdf_table *table);

    static double interpolate(double x, double x1, double y1, double x2, double y2);

    /* generate a random floating point number from min to max */
    static double rand_range(double min, double max);

    static int rand_range(int min, int max);

    static double poission_gen_interval(double avg_rate);

    static double gen_requestRate(double load, double link_capacity, struct cdf_table *table);

    /*assign priority according to the flow size*/
    static int gen_priority(int flowsize);

    static double gen_random();
};

} // namespace ns3

#endif