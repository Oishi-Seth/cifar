#define inElem_MAX 10500

template<
	unsigned KerDim_curr,
	unsigned IFMCH_curr,
	unsigned IFMDim_curr,
	unsigned OFMCH_curr,
	unsigned OFMDim_curr,
	unsigned padValue
>
void SCIG(
	hls::stream<AXI_VAL> & in, 
	hls::stream<AXI_VAL> & out
) {
	AXI_VAL valIn, valOut;

	// first two data as row_size and col_size
	valIn = in.read();
	unsigned status = (unsigned)valIn;
	out.write(valIn);

	valIn = in.read();
	unsigned batch_size = (unsigned)valIn;
	out.write(valIn);

	valIn = in.read();
	unsigned ConvKernelDim = (unsigned)valIn;
	out.write(valIn);

	valIn = in.read();
	unsigned IFMChannels = (unsigned)valIn;
	out.write(valIn);

	valIn = in.read();
	unsigned IFMDim = (unsigned)valIn;
	out.write(valIn);

	valIn = in.read();
	unsigned OFMChannels = (unsigned)valIn;
	out.write(valIn);

	valIn = in.read();
	unsigned OFMDim = (unsigned)valIn;
	out.write(valIn);

	valIn = in.read();
	unsigned PadDim = (unsigned)valIn;
	out.write(valIn);

	if (status == 0){

		unsigned int IFMPadDim = IFMDim_curr + 2*padValue;
		unsigned int IFMPadDimSqrt = IFMPadDim*IFMPadDim;
		ap_int<InpWidth> inputBuf[inElem_MAX];
	#pragma HLS bind_storage variable=inputBuf type=RAM_S2P impl=BRAM
	#pragma HLS ARRAY_PARTITION variable=inputBuf cyclic factor=8 dim=1
		int IFMLoopBound = IFMDim_curr + padValue;
		unsigned int additional_lines = IFMPadDimSqrt / (OFMDim_curr * KerDim_curr * KerDim_curr);
		unsigned int Initial_lines = ((IFMPadDim) < (OFMDim_curr * KerDim_curr * KerDim_curr)) ? (KerDim_curr+1) : (KerDim_curr + additional_lines - IFMDim_curr);
		unsigned int Initial_buffer = MIN(Initial_lines * IFMPadDim, IFMPadDim * IFMPadDim-1);
		unsigned int baseIter = Initial_buffer + (OFMDim_curr * OFMDim_curr * KerDim_curr * KerDim_curr);

		unsigned int inp = 0, oy = 0, ox = 0, ky = 0, kx = 0;
		int inp_i = -padValue, inp_j = -padValue;

		unsigned int baseIterBound = baseIter * batch_size;
	#pragma HLS bind_storage variable=baseIterBound type=ram_2p impl=auto
		for (unsigned int i = 0; i < baseIter * batch_size; i++) {
		#pragma HLS PIPELINE II=4
		#pragma HLS DEPENDENCE variable=inputBuf inter false
			if (inp < IFMPadDimSqrt) {
				ap_uint<InpWidth> inElem[256];
			#pragma HLS bind_storage variable=inElem type=RAM_S2P impl=LUTRAM
			#pragma HLS ARRAY_PARTITION variable=inElem complete dim=1
				if ((inp_i < 0) || (inp_j < 0) || (inp_i >= IFMDim_curr) || (inp_j >= IFMDim_curr)) {
					for (unsigned int inp_ch = 0; inp_ch < IFMCH_curr; inp_ch++) {
						inElem[inp_ch] = padValue;
					}
				} else {
					for (unsigned int inp_ch = 0; inp_ch < IFMCH_curr; inp_ch++) {
						AXI_VAL inElem_tmp = in.read();
						inElem[inp_ch] = inElem_tmp;
					}
				}
				for (unsigned int inp_ch = 0; inp_ch < IFMCH_curr; inp_ch++) {
					inputBuf[inp * IFMCH_curr + inp_ch] = inElem[inp_ch];
				}
				inp++;
				inp_j++;
				if (inp_j == IFMLoopBound) {
					inp_j = -padValue;
					inp_i++;
					if (inp_i == IFMLoopBound) {
						inp_i = -padValue;
					}
				}
			}
			if (inp > Initial_buffer) {
				unsigned int input_base = oy * IFMPadDim + ox;
				unsigned int input_ind = input_base + ky * IFMPadDim + kx;
				for (unsigned int inp_ch = 0; inp_ch < IFMCH_curr; inp_ch++) {
					out.write(inputBuf[input_ind * IFMCH_curr + inp_ch]);
				}
				kx++;
				if (kx == KerDim_curr) {
					kx = 0;
					ky++;
					if (ky == KerDim_curr) {
						ky = 0;
						ox++;
						if (ox == OFMDim_curr) {
							ox = 0;
							oy++;
							if (oy == OFMDim_curr) {
								oy = 0;
								inp = 0;
							}
						}
					}
				}
			}
		}
	} else {
		unsigned int KER_size_0 = OFMChannels * ConvKernelDim;
		unsigned int KER_size_1 = KER_size_0 * ConvKernelDim;
		unsigned int KER_bound = KER_size_1 * IFMChannels;
	#pragma HLS bind_storage variable=KER_size_0 type=ram_2p impl=auto
	#pragma HLS bind_storage variable=KER_size_1 type=ram_2p impl=auto
	#pragma HLS bind_storage variable=KER_bound type=ram_2p impl=auto
		for (unsigned int i = 0; i < KER_bound; i++) {
		#pragma HLS PIPELINE II=2
			valIn = in.read();
			out.write(valIn);
		}
	}
}
