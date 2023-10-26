import numpy as np
import scipy.signal as sc
import logging

def envelope_coeff(tau, sr):
    """
    Filtro de envolvente energética.

              z
    ----------------------
    (1 + fs*tau)z - fs*tau

    """
    logging.info("Calculating coefficients of temporal weighting: {} filter.".format(tau))

    if tau == "fast":
        tau = 0.125
    elif tau == "slow":
        tau = 1.
    else:
        logging.error("Wrong tau value. Indicate 'fast' or 'slow'.")
    
    # filtro para obtener envolvente energética:
    b = [1.,0.]  # coeficientes numerador
    a = [(1 + sr*tau), -sr*tau]  # coeficientes denominador
    
    return b, a

def A_weighting_coeff(sr):
        
    logging.info("Calculating coefficients of frequential weighting filter.")
    fa = 10**2.45
    d = np.sqrt(0.5)
    fr = 1000
    fh = 10**3.9
    fl = 10**1.5
    c = (fl**2)*(fh**2)
    b = (1/(1-d))*((fr**2)+(fl**2)*(fh**2)/(fr**2)-d*((fl**2)+(fh**2)))
    f1 = np.sqrt((-b-np.sqrt((b**2)-4*c))/2)
    f2 = ((3-np.sqrt(5))/2)*fa
    f3 = ((3+np.sqrt(5))/2)*fa
    f4 = np.sqrt((-b+np.sqrt((b**2)-4*c))/2)
    # Definición de las frecuencias de los polos y ceros.
    zs = [0, 0, 0, 0]
    ps = [-2*np.pi*f1,
         -2*np.pi*f1,
         -2*np.pi*f4,
         -2*np.pi*f4,
         -2*np.pi*f2,
         -2*np.pi*f3]
    ks = 1/np.abs(sc.freqs_zpk(zs,ps,1, worN=[2*np.pi*1000])[1][0])

    pz = [np.exp(pole * (1 / sr)) for pole in ps]
    zz = [np.exp(zero * (1 / sr)) for zero in zs]
    kz = 1/np.abs(sc.freqz_zpk(zz,pz,1, worN=[1000], fs=sr)[1][0])


    fn = sr/2
    H = np.abs(sc.freqs_zpk(zs,ps,ks, worN=[2*np.pi*fn/2])[1] /
               sc.freqz_zpk(zz,pz,kz, worN=[fn/2], fs=sr)[1]) 
    b1 = ( 1 + np.sqrt(2 * H[0]**2 - 1, dtype=complex) ) / 2
    if b1.imag!=0:
        b1 = np.abs(b1)
    else:
        b1 = b1.real
    b0 = 1 - b1
    fir=[b0,b1]
    '''
    H = np.abs(sc.freqs_zpk(zs,ps,ks, worN=[2*np.pi*fn/3,2*2*np.pi*fn/3])[1] /
               sc.freqz_zpk(zz,pz,kz, worN=[fn/3,2*fn/3], fs=sr)[1])  
    b1 = ( 1 - np.sqrt( 1 - 2 * H[0]**2 + 2 * H[1], dtype=complex) ) / 2
    if b1.imag!=0: b1 = np.abs(b1)
    b2 = (3*(1-b1) - np.sqrt(-3 + 12 * H[0] ** 2 - 6 * b1 - 3 * b1 ** 2, dtype=complex) ) / 6
    if b2.imag!=0: b2 = np.abs(b2)
    b0 = 1 - b1 - b2
    if b0.imag!=0: b0 = np.abs(b0)
    fir=[b0,b1,b2]
    '''

    b, a = sc.zpk2tf(zz, pz, kz)
    b = np.polymul(b,fir)
    b /= np.abs(sc.freqz(b,a, worN=[1000], fs=sr)[1][0])
    return b, a

def comp_coeff(sr):

    try:
        logging.info('Loading compensation filter coefficients.')        
        a = np.load('/home/noisen/source/coeff_compensation_a.npy')
        b = np.load('/home/noisen/source/coeff_compensation_b.npy')
        logging.info('Length of numerator coefficients: ' + str(len(b)))
        logging.info('Length of denominator coefficients: ' + str(len(a)))

        sr_comp = np.load('/home/noisen/source/compensation_sr.npy')
        if sr_comp != sr:
            logging.error('Sample rate of reference and measure mismatch.')
    except:
        logging.error("Couldn't load compensation filter coefficients. Setting den=1 and num=[1,0].")
        a = 1
        b = [1,0]

    return b, a

def bands_coeff(sr):
    #G = 10**.3
    G = 2
    central_bands = 1000*(G**(np.linspace(-5,4,10)))
    #central_bands = np.array([31.5,63,125,250,500,1000,2000,4000,8000,16000])
    bands_inf = central_bands*(G**(-.5))
    bands_sup = central_bands*(G**(.5))
    band_filters = []
    for idx, _ in enumerate(central_bands):
        sos = sc.butter(6, [bands_inf[idx], bands_sup[idx]], btype='bandpass', output="sos", fs=sr)
        band_filters.append(sos)
    return band_filters

class filters:

    ''' Class that contains methods to filter the signal.'''
    
    def __init__(self, sr):
        '''
        Constructor of the filters class.

        Args:
            conf_data (dict): A dictionary containing configuration data.
        '''
        self.b_fast, self.a_fast = envelope_coeff('fast', sr)
        self.b_slow, self.a_slow = envelope_coeff('slow', sr)
        self.b_pond, self.a_pond = A_weighting_coeff(sr)
        self.b_comp, self.a_comp = comp_coeff(sr)
        self.bands_sos = bands_coeff(sr)

        self.zi_fast = sc.lfilter_zi(self.b_fast, self.a_fast)*0
        self.zi_slow = sc.lfilter_zi(self.b_slow, self.a_slow)*0
        self.zi_pond = sc.lfilter_zi(self.b_pond, self.a_pond)*0
        self.zi_comp = sc.lfilter_zi(self.b_comp, self.a_comp)*0
        self.zi_bands = [sc.sosfilt_zi(coeff)*0 for coeff in self.bands_sos]

    def octave_bands(self, x):
        logging.debug("Applying octave band filters.")
        filtered_bands = []
        for idx, _ in enumerate(self.bands_sos):
            filtered_signal, self.zi_bands[idx] = sc.sosfilt(self.bands_sos[idx], x, zi = self.zi_bands[idx])
            filtered_bands.append(filtered_signal)
        return filtered_bands

    def A_weighting(self, x):
        '''
        Method that takes an audio signal and apply the A weighting filter.

        Args:
            x (numpy array): audio signal to filter
        Returns: 
            numpy array: audio signal with the A weighting filter.
        '''
        logging.debug("Applying A weighting.")
        filtered_signal, self.zi_pond = sc.lfilter(self.b_pond, self.a_pond, x, zi = self.zi_pond)
        return filtered_signal


    def envelope(self, x, tau):
        '''
        Method that takes an audio signal and apply a temporal weighting according to the configuration file.

        Args:
            x (numpy array): audio signal to filter.
        Returns:
            numpy array: Energy envelope.
        '''
        if tau=='slow':
            logging.debug("Calculating energy envelope using slow integration.")
            filtered_signal, self.zi_slow = sc.lfilter(self.b_slow, self.a_slow, np.power(x,2), zi = self.zi_slow)
            return np.abs( filtered_signal )
        elif tau=='fast':
            logging.debug("Calculating energy envelope using fast integration.")
            filtered_signal, self.zi_fast = sc.lfilter(self.b_fast, self.a_fast, np.power(x,2), zi = self.zi_fast)
            return np.abs( filtered_signal )
        else:
            logging.debug("Calculating energy envelope using linear integration.")
            return np.power(x,2)

    
    
    def compensation(self, x):
        '''
        Method that takes an audio signal and apply a frequency compensation based on the compensation coefficients.

        Args:
            x (numpy array): audio signal to filter.
        Returns:
            numpy array: Compensated signal.
        '''
        logging.debug("Compensating microphone frequency response.")
        filtered_signal, self.zi_comp = sc.lfilter(self.b_comp, self.a_comp, x, zi = self.zi_comp)
        return filtered_signal

print(envelope_coeff(tau = 1.0,sr = 48000))