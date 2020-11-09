#include "mtu-utility.h"
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

namespace ns3
{
/* initialize a CDF distribution*/
void MtuUtility::init_cdf(struct cdf_table *table)
{
    if (!table)
        return;

    table->entries = (struct cdf_entry *)malloc(CDF_TABLE_ENTRY * sizeof(struct cdf_entry));
    table->num_entry = 0;
    table->max_entry = CDF_TABLE_ENTRY;
    table->min_cdf = 0;
    table->max_cdf = 1;

    if (!(table->entries))
        perror("Error:malloc entries in init_cdf()");
}

/*free resources of a CDF distribution*/
void MtuUtility::free_cdf(struct cdf_table *table)
{
    if (table)
        free(table->entries);
}

/*get CDF distribution from a given file*/
void MtuUtility::load_cdf(struct cdf_table *table, const char *file_name)
{
    FILE *file = NULL;
    char line[256] = {0};
    struct cdf_entry *e = NULL;
    int i = 0;

    if (!table)
        return;
    file = fopen(file_name, "r");
    if (!file)
        perror("Error: open the CDF file in load_cdf()");

    while (fgets(line, sizeof(line), file))
    {
        /*resize entries*/
        if (table->num_entry >= table->max_entry)
        {
            table->max_entry *= 2;
            e = (struct cdf_entry *)malloc(table->max_entry * sizeof(struct cdf_entry));
            if (!e)
                perror("Error: malloc entries in load_cdf()");
            for (i = 0; i < table->num_entry; i++)
                e[i] = table->entries[i];
            free(table->entries);
            table->entries = e;
        }

        /*read from char* */
        sscanf(line, "%lf %lf", &(table->entries[table->num_entry].value), &(table->entries[table->num_entry].cdf));
        if (table->min_cdf > table->entries[table->num_entry].cdf)
            table->min_cdf = table->entries[table->num_entry].cdf;
        if (table->max_cdf < table->entries[table->num_entry].cdf)
            table->max_cdf = table->entries[table->num_entry].cdf;

        table->num_entry++;
    }
    fclose(file);
}

/* print CDF distribution information */
void MtuUtility::print_cdf(struct cdf_table *table)
{
    int i = 0;

    if (!table)
        return;

    for (i = 0; i < table->num_entry; i++)
        printf("%.2f %.2f\n", table->entries[i].value, table->entries[i].cdf);
}

/* get average value of CDF distribution */
double
MtuUtility::avg_cdf(struct cdf_table *table)
{
    int i = 0;
    double avg = 0;
    double value, prob;

    if (!table)
        return 0;

    for (i = 0; i < table->num_entry; i++)
    {
        if (i == 0)
        {
            value = table->entries[i].value / 2;
            prob = table->entries[i].cdf;
        }
        else
        {
            value = (table->entries[i].value + table->entries[i - 1].value) / 2;
            prob = table->entries[i].cdf - table->entries[i - 1].cdf;
        }
        avg += (value * prob);
    }

    return avg;
}

/* */
double
MtuUtility::interpolate(double x, double x1, double y1, double x2, double y2)
{
    if (x1 == x2)
        return (y1 + y2) / 2;
    else
        return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
}

/* generate a random floating point number from min to max */
double
MtuUtility::rand_range(double min, double max)
{
    return min + rand() * (max - min) / RAND_MAX;
}

int MtuUtility::rand_range(int min, int max)
{
    return min + ((double)max - min) * rand() / RAND_MAX;
}

/* generate a random value based on CDF distribution */
double
MtuUtility::gen_random_cdf(struct cdf_table *table)
{
    int i = 0;
    double x = rand_range(table->min_cdf, table->max_cdf);
    /* printf("%f %f %f\n", x, table->min_cdf, table->max_cdf); */

    if (!table)
        return 0;

    for (i = 0; i < table->num_entry; i++)
    {
        if (x <= table->entries[i].cdf)
        {
            if (i == 0)
                return interpolate(x, 0, 0, table->entries[i].cdf, table->entries[i].value);
            else
                return interpolate(x, table->entries[i - 1].cdf, table->entries[i - 1].value, table->entries[i].cdf, table->entries[i].value);
        }
    }

    return table->entries[table->num_entry - 1].value;
}

double
MtuUtility::poission_gen_interval(double avg_rate)
{
    if (avg_rate > 0)
        return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
    else
        return 0;
}

/*calculate request rate*/
double
MtuUtility::gen_requestRate(double load, double link_capacity, struct cdf_table *table)
{
    return load * link_capacity * 8 / 2 / (8 * avg_cdf(table)) / 8;
}

int MtuUtility::gen_priority(int flowsize)
{
    int priority;
    //0-100k 0 100k-10M 1 10M- 2
    if (flowsize < 102400)
    {
        priority = 0;
    }
    else if (flowsize < 10485760)
    {
        priority = 1;
    }
    else
    {
        priority = 2;
    }
    return priority;
}

double MtuUtility::gen_random()
{
    srand(time(0));
    double rand_value = rand() % 100 / double(101);
    return rand_value;
}
} // namespace ns3
