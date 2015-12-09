#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "errortests.h"
#include "fastq.h"
#include "histogram.h"
#include "histogramtests.h"
#include "bloomfilter.h"
#include "minsketch.h"
#include "tests.h"
#include "hash.h"
#include "error.h"

#define KMER_SIZE 10
#define FASTQ_FILE "reads.fastq"
#define FAKE_KMER "ATATATATAT"

// error_test1 flags
#define DEBUG_STATS false
#define DEBUG_KMERS false
#define DEBUG_LIERS false
#define DEBUG_FAKES false
#define DEBUG_FAKER false

// error_test2 flags
#define DEBUG_SEQS  false

#define USING(x,y,z) histogram *h = histogram_new(x, y); ASSERT(h != NULL); z; histogram_free(h);
#define READ(...) histogram_read(h, FASTQ_FILE, KMER_SIZE)
#define COUNT(x) histogram_count(h, x)
#define FOR_EACH_KMER(x, y) fastq_for_each_kmer(FASTQ_FILE, KMER_SIZE, x, y);
#define LOAD_FACTOR(...) histogram_load_factor(h)
#define ERROR_DETECT(sequence, cutoff) error_detect(h, sequence, KMER_SIZE, cutoff)

TEST(error_test1(), {
	USING(MINSKETCH, minsketch_new(9000, 1), {
		READ();
		int n = 0;
		float mean = 0.0;
		FOR_EACH_KMER(kmer, {
			n++;
			mean += COUNT(kmer);
		});
		mean /= (float)n;
		float variance = 0.0f;
		FOR_EACH_KMER(kmer, {
			variance += powf(COUNT(kmer) - mean, 2);
		});
		variance /= n - 1;
		float stddev = sqrtf(variance);
		int outliers = 0;
		FOR_EACH_KMER(kmer, {
			int count = COUNT(kmer);
			#if DEBUG_KMERS
				PRINT("%s", kmer);
			#endif
			if (count <= ceil(mean - 2 * stddev)) {
				#if DEBUG_LIERS && DEBUG_KMERS == false
					PRINT("%s: %u", kmer, count);
				#endif
				outliers++;
			}
		});
		#if DEBUG_STATS
			PRINT("len: %d", n);
			PRINT("mean: %f", mean);
			PRINT("stddev: %f", stddev);
			PRINT("outliers: %d (%f%%)", outliers, 100*outliers/(float)(n));
			PRINT("load factor: %f%%", LOAD_FACTOR());
		#endif
		#if DEBUG_FAKES
			char temp[KMER_SIZE * 2 + 1];
			memset(temp, 0, KMER_SIZE * 2 + 1);
			PRINT("fake reads:")
			for (int i = 0; i < KMER_SIZE * 2; i++) {
				temp[i] = 'A' + i;
				PRINT("  %s: %d", temp, COUNT(temp));
			}
		#endif
		#if DEBUG_FAKER
			PRINT("%s: %d", FAKE_KMER, COUNT(FAKE_KMER));
		#endif
	});
})

TEST(error_test2(), {
	USING(MINSKETCH, minsketch_new(9000, 1), {
		READ();
		fastq *f = fastq_new(FASTQ_FILE); 
		while(fastq_read_line(f)) {
			if (ERROR_DETECT(f->sequence, 1)) {
				#if DEBUG_SEQS
					PRINT("%s", f->sequence);
				#endif
			}
		}
	});
});

TEST(error_test(), {
	ASSERT(error_test1() == true);
	ASSERT(error_test2() == true);
})