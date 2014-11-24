/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "rpcsvc/rstat.h"

bool_t
xdr_rstat_timeval (XDR *xdrs, rstat_timeval *objp)
{
	register int32_t *buf;

	 if (!xdr_u_int (xdrs, &objp->tv_sec))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->tv_usec))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_statstime (XDR *xdrs, statstime *objp)
{
	register int32_t *buf;

	int i;

	if (xdrs->x_op == XDR_ENCODE) {
		buf = XDR_INLINE (xdrs, (10 + ( CPUSTATES  + DK_NDRIVE  + 3 )) * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_vector (xdrs, (char *)objp->cp_time, CPUSTATES,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->dk_xfer, DK_NDRIVE,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_intr))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ipackets))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ierrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_oerrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_collisions))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_swtch))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->avenrun, 3,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;

		} else {
		{
			register int *genp;

			for (i = 0, genp = objp->cp_time;
				i < CPUSTATES; ++i) {
				IXDR_PUT_LONG(buf, *genp++);
			}
		}
		{
			register int *genp;

			for (i = 0, genp = objp->dk_xfer;
				i < DK_NDRIVE; ++i) {
				IXDR_PUT_LONG(buf, *genp++);
			}
		}
		IXDR_PUT_U_LONG(buf, objp->v_pgpgin);
		IXDR_PUT_U_LONG(buf, objp->v_pgpgout);
		IXDR_PUT_U_LONG(buf, objp->v_pswpin);
		IXDR_PUT_U_LONG(buf, objp->v_pswpout);
		IXDR_PUT_U_LONG(buf, objp->v_intr);
		IXDR_PUT_LONG(buf, objp->if_ipackets);
		IXDR_PUT_LONG(buf, objp->if_ierrors);
		IXDR_PUT_LONG(buf, objp->if_oerrors);
		IXDR_PUT_LONG(buf, objp->if_collisions);
		IXDR_PUT_U_LONG(buf, objp->v_swtch);
		{
			register int *genp;

			for (i = 0, genp = objp->avenrun;
				i < 3; ++i) {
				IXDR_PUT_LONG(buf, *genp++);
			}
		}
		}
		 if (!xdr_rstat_timeval (xdrs, &objp->boottime))
			 return FALSE;
		 if (!xdr_rstat_timeval (xdrs, &objp->curtime))
			 return FALSE;
		 if (!xdr_int (xdrs, &objp->if_opackets))
			 return FALSE;
		return TRUE;
	} else if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE (xdrs, (10 + ( CPUSTATES  + DK_NDRIVE  + 3 )) * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_vector (xdrs, (char *)objp->cp_time, CPUSTATES,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->dk_xfer, DK_NDRIVE,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_intr))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ipackets))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ierrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_oerrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_collisions))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_swtch))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->avenrun, 3,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;

		} else {
		{
			register int *genp;

			for (i = 0, genp = objp->cp_time;
				i < CPUSTATES; ++i) {
				*genp++ = IXDR_GET_LONG(buf);
			}
		}
		{
			register int *genp;

			for (i = 0, genp = objp->dk_xfer;
				i < DK_NDRIVE; ++i) {
				*genp++ = IXDR_GET_LONG(buf);
			}
		}
		objp->v_pgpgin = IXDR_GET_U_LONG(buf);
		objp->v_pgpgout = IXDR_GET_U_LONG(buf);
		objp->v_pswpin = IXDR_GET_U_LONG(buf);
		objp->v_pswpout = IXDR_GET_U_LONG(buf);
		objp->v_intr = IXDR_GET_U_LONG(buf);
		objp->if_ipackets = IXDR_GET_LONG(buf);
		objp->if_ierrors = IXDR_GET_LONG(buf);
		objp->if_oerrors = IXDR_GET_LONG(buf);
		objp->if_collisions = IXDR_GET_LONG(buf);
		objp->v_swtch = IXDR_GET_U_LONG(buf);
		{
			register int *genp;

			for (i = 0, genp = objp->avenrun;
				i < 3; ++i) {
				*genp++ = IXDR_GET_LONG(buf);
			}
		}
		}
		 if (!xdr_rstat_timeval (xdrs, &objp->boottime))
			 return FALSE;
		 if (!xdr_rstat_timeval (xdrs, &objp->curtime))
			 return FALSE;
		 if (!xdr_int (xdrs, &objp->if_opackets))
			 return FALSE;
	 return TRUE;
	}

	 if (!xdr_vector (xdrs, (char *)objp->cp_time, CPUSTATES,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->dk_xfer, DK_NDRIVE,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pgpgin))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pgpgout))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pswpin))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pswpout))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_intr))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_ipackets))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_ierrors))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_oerrors))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_collisions))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_swtch))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->avenrun, 3,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	 if (!xdr_rstat_timeval (xdrs, &objp->boottime))
		 return FALSE;
	 if (!xdr_rstat_timeval (xdrs, &objp->curtime))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_opackets))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_statsswtch (XDR *xdrs, statsswtch *objp)
{
	register int32_t *buf;

	int i;

	if (xdrs->x_op == XDR_ENCODE) {
		buf = XDR_INLINE (xdrs, (10 + ( CPUSTATES  + DK_NDRIVE  + 3 )) * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_vector (xdrs, (char *)objp->cp_time, CPUSTATES,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->dk_xfer, DK_NDRIVE,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_intr))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ipackets))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ierrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_oerrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_collisions))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_swtch))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->avenrun, 3,
				sizeof (u_int), (xdrproc_t) xdr_u_int))
				 return FALSE;

		} else {
		{
			register int *genp;

			for (i = 0, genp = objp->cp_time;
				i < CPUSTATES; ++i) {
				IXDR_PUT_LONG(buf, *genp++);
			}
		}
		{
			register int *genp;

			for (i = 0, genp = objp->dk_xfer;
				i < DK_NDRIVE; ++i) {
				IXDR_PUT_LONG(buf, *genp++);
			}
		}
		IXDR_PUT_U_LONG(buf, objp->v_pgpgin);
		IXDR_PUT_U_LONG(buf, objp->v_pgpgout);
		IXDR_PUT_U_LONG(buf, objp->v_pswpin);
		IXDR_PUT_U_LONG(buf, objp->v_pswpout);
		IXDR_PUT_U_LONG(buf, objp->v_intr);
		IXDR_PUT_LONG(buf, objp->if_ipackets);
		IXDR_PUT_LONG(buf, objp->if_ierrors);
		IXDR_PUT_LONG(buf, objp->if_oerrors);
		IXDR_PUT_LONG(buf, objp->if_collisions);
		IXDR_PUT_U_LONG(buf, objp->v_swtch);
		{
			register u_int *genp;

			for (i = 0, genp = objp->avenrun;
				i < 3; ++i) {
				IXDR_PUT_U_LONG(buf, *genp++);
			}
		}
		}
		 if (!xdr_rstat_timeval (xdrs, &objp->boottime))
			 return FALSE;
		 if (!xdr_int (xdrs, &objp->if_opackets))
			 return FALSE;
		return TRUE;
	} else if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE (xdrs, (10 + ( CPUSTATES  + DK_NDRIVE  + 3 )) * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_vector (xdrs, (char *)objp->cp_time, CPUSTATES,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->dk_xfer, DK_NDRIVE,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_intr))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ipackets))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ierrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_oerrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_collisions))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_swtch))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->avenrun, 3,
				sizeof (u_int), (xdrproc_t) xdr_u_int))
				 return FALSE;

		} else {
		{
			register int *genp;

			for (i = 0, genp = objp->cp_time;
				i < CPUSTATES; ++i) {
				*genp++ = IXDR_GET_LONG(buf);
			}
		}
		{
			register int *genp;

			for (i = 0, genp = objp->dk_xfer;
				i < DK_NDRIVE; ++i) {
				*genp++ = IXDR_GET_LONG(buf);
			}
		}
		objp->v_pgpgin = IXDR_GET_U_LONG(buf);
		objp->v_pgpgout = IXDR_GET_U_LONG(buf);
		objp->v_pswpin = IXDR_GET_U_LONG(buf);
		objp->v_pswpout = IXDR_GET_U_LONG(buf);
		objp->v_intr = IXDR_GET_U_LONG(buf);
		objp->if_ipackets = IXDR_GET_LONG(buf);
		objp->if_ierrors = IXDR_GET_LONG(buf);
		objp->if_oerrors = IXDR_GET_LONG(buf);
		objp->if_collisions = IXDR_GET_LONG(buf);
		objp->v_swtch = IXDR_GET_U_LONG(buf);
		{
			register u_int *genp;

			for (i = 0, genp = objp->avenrun;
				i < 3; ++i) {
				*genp++ = IXDR_GET_U_LONG(buf);
			}
		}
		}
		 if (!xdr_rstat_timeval (xdrs, &objp->boottime))
			 return FALSE;
		 if (!xdr_int (xdrs, &objp->if_opackets))
			 return FALSE;
	 return TRUE;
	}

	 if (!xdr_vector (xdrs, (char *)objp->cp_time, CPUSTATES,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->dk_xfer, DK_NDRIVE,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pgpgin))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pgpgout))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pswpin))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pswpout))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_intr))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_ipackets))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_ierrors))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_oerrors))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_collisions))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_swtch))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->avenrun, 3,
		sizeof (u_int), (xdrproc_t) xdr_u_int))
		 return FALSE;
	 if (!xdr_rstat_timeval (xdrs, &objp->boottime))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_opackets))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_stats (XDR *xdrs, stats *objp)
{
	register int32_t *buf;

	int i;

	if (xdrs->x_op == XDR_ENCODE) {
		buf = XDR_INLINE (xdrs, (10 +  CPUSTATES  + DK_NDRIVE )* BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_vector (xdrs, (char *)objp->cp_time, CPUSTATES,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->dk_xfer, DK_NDRIVE,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_intr))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ipackets))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ierrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_oerrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_collisions))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_opackets))
				 return FALSE;
		} else {
			{
				register int *genp;

				for (i = 0, genp = objp->cp_time;
					i < CPUSTATES; ++i) {
					IXDR_PUT_LONG(buf, *genp++);
				}
			}
			{
				register int *genp;

				for (i = 0, genp = objp->dk_xfer;
					i < DK_NDRIVE; ++i) {
					IXDR_PUT_LONG(buf, *genp++);
				}
			}
			IXDR_PUT_U_LONG(buf, objp->v_pgpgin);
			IXDR_PUT_U_LONG(buf, objp->v_pgpgout);
			IXDR_PUT_U_LONG(buf, objp->v_pswpin);
			IXDR_PUT_U_LONG(buf, objp->v_pswpout);
			IXDR_PUT_U_LONG(buf, objp->v_intr);
			IXDR_PUT_LONG(buf, objp->if_ipackets);
			IXDR_PUT_LONG(buf, objp->if_ierrors);
			IXDR_PUT_LONG(buf, objp->if_oerrors);
			IXDR_PUT_LONG(buf, objp->if_collisions);
			IXDR_PUT_LONG(buf, objp->if_opackets);
		}
		return TRUE;
	} else if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE (xdrs, (10 +  CPUSTATES  + DK_NDRIVE )* BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_vector (xdrs, (char *)objp->cp_time, CPUSTATES,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_vector (xdrs, (char *)objp->dk_xfer, DK_NDRIVE,
				sizeof (int), (xdrproc_t) xdr_int))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pgpgout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpin))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_pswpout))
				 return FALSE;
			 if (!xdr_u_int (xdrs, &objp->v_intr))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ipackets))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_ierrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_oerrors))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_collisions))
				 return FALSE;
			 if (!xdr_int (xdrs, &objp->if_opackets))
				 return FALSE;
		} else {
			{
				register int *genp;

				for (i = 0, genp = objp->cp_time;
					i < CPUSTATES; ++i) {
					*genp++ = IXDR_GET_LONG(buf);
				}
			}
			{
				register int *genp;

				for (i = 0, genp = objp->dk_xfer;
					i < DK_NDRIVE; ++i) {
					*genp++ = IXDR_GET_LONG(buf);
				}
			}
			objp->v_pgpgin = IXDR_GET_U_LONG(buf);
			objp->v_pgpgout = IXDR_GET_U_LONG(buf);
			objp->v_pswpin = IXDR_GET_U_LONG(buf);
			objp->v_pswpout = IXDR_GET_U_LONG(buf);
			objp->v_intr = IXDR_GET_U_LONG(buf);
			objp->if_ipackets = IXDR_GET_LONG(buf);
			objp->if_ierrors = IXDR_GET_LONG(buf);
			objp->if_oerrors = IXDR_GET_LONG(buf);
			objp->if_collisions = IXDR_GET_LONG(buf);
			objp->if_opackets = IXDR_GET_LONG(buf);
		}
	 return TRUE;
	}

	 if (!xdr_vector (xdrs, (char *)objp->cp_time, CPUSTATES,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->dk_xfer, DK_NDRIVE,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pgpgin))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pgpgout))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pswpin))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_pswpout))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->v_intr))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_ipackets))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_ierrors))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_oerrors))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_collisions))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->if_opackets))
		 return FALSE;
	return TRUE;
}
