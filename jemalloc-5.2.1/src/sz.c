#include "jemalloc/internal/jemalloc_preamble.h"
#include "jemalloc/internal/sz.h"

#ifdef GRANDSTREAM_NETWORKS
#include "jemalloc/internal/jelog.h"
#endif

JEMALLOC_ALIGNED(CACHELINE)
size_t sz_pind2sz_tab[SC_NPSIZES+1];

static void
sz_boot_pind2sz_tab(const sc_data_t *sc_data) {
	int pind = 0;
	for (unsigned i = 0; i < SC_NSIZES; i++) {
		const sc_t *sc = &sc_data->sc[i];
		if (sc->psz) {
			sz_pind2sz_tab[pind] = (ZU(1) << sc->lg_base)
			    + (ZU(sc->ndelta) << sc->lg_delta);
#ifdef GRANDSTREAM_NETWORKS
			jelog(1, "Boot page index to table, SC_NSIZES: %ld, sz_pind2sz_tab[%d]: %ld", SC_NSIZES, pind, sz_pind2sz_tab[pind]);
#endif

			pind++;
		}
	}
	for (int i = pind; i <= (int)SC_NPSIZES; i++) {
		sz_pind2sz_tab[pind] = sc_data->large_maxclass + PAGE;
#ifdef GRANDSTREAM_NETWORKS
		jelog(1, "Boot page index to table, SC_NPSIZES: %ld, sz_pind2sz_tab[%d]: %ld", SC_NPSIZES, pind, sz_pind2sz_tab[pind]);
#endif

	}
}

JEMALLOC_ALIGNED(CACHELINE)
size_t sz_index2size_tab[SC_NSIZES];

static void
sz_boot_index2size_tab(const sc_data_t *sc_data) {
	for (unsigned i = 0; i < SC_NSIZES; i++) {
		const sc_t *sc = &sc_data->sc[i];
		sz_index2size_tab[i] = (ZU(1) << sc->lg_base)
		    + (ZU(sc->ndelta) << (sc->lg_delta));
#ifdef GRANDSTREAM_NETWORKS
		jelog(1, "Boot index2size table, SC_NSIZES: %ld, sz_index2size_tab[%d]: %d",
			SC_NSIZES, i, sz_index2size_tab[i]);
#endif
	}
}

/*
 * To keep this table small, we divide sizes by the tiny min size, which gives
 * the smallest interval for which the result can change.
 */
JEMALLOC_ALIGNED(CACHELINE)
uint8_t sz_size2index_tab[(SC_LOOKUP_MAXCLASS >> SC_LG_TINY_MIN) + 1];

static void
sz_boot_size2index_tab(const sc_data_t *sc_data) {
	size_t dst_max = (SC_LOOKUP_MAXCLASS >> SC_LG_TINY_MIN) + 1;
	size_t dst_ind = 0;
	for (unsigned sc_ind = 0; sc_ind < SC_NSIZES && dst_ind < dst_max;
	    sc_ind++) {
		const sc_t *sc = &sc_data->sc[sc_ind];
		size_t sz = (ZU(1) << sc->lg_base)
		    + (ZU(sc->ndelta) << sc->lg_delta);
		size_t max_ind = ((sz + (ZU(1) << SC_LG_TINY_MIN) - 1)
				   >> SC_LG_TINY_MIN);
		for (; dst_ind <= max_ind && dst_ind < dst_max; dst_ind++) {
			sz_size2index_tab[dst_ind] = sc_ind;
#ifdef GRANDSTREAM_NETWORKS
			jelog(1, "Boot size2index table, dst_max: %ld, sz_size2index_tab[%d]: %d",
				dst_max, dst_ind, sz_size2index_tab[dst_ind]);
#endif
		}
	}
}

void
sz_boot(const sc_data_t *sc_data) {
	sz_boot_pind2sz_tab(sc_data);
	sz_boot_index2size_tab(sc_data);
	sz_boot_size2index_tab(sc_data);
}
