#!/usr/bin/python
# (C) 2004-2012, Markus Wildi, wildi.markus@bluewin.ch
#
#   Measure the position of the our axis based on E.S. King, A.A. Rambaut
#   
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software Foundation,
#   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#   Or visit http://www.gnu.org/licenses/gpl.html.

__author__ = 'wildi.markus@bluewin.ch'

import os
import sys
import rts2pa
import ephem
import threading
import Queue
import math
import rts2.scriptcomm
r2c= rts2.scriptcomm.Rts2Comm()
import pyfits
import rts2.astrometry
import rts2.libnova
import sidereal


class KingA():
    """Calculate the HA, lambda based on E.S. King's method """
    def __init__(self, results=None): # results is a list of SolverResult
        self.results= results

#        self.tau= sidereal.raToHourAngle(1., 2., 3.)     # HA of image 
#        self.lat=math.radian(lat)   
#        self.dX= math.radian(result2.RA - result2.RA)
#        self.dY= math.radian(result2.DEC - result2.DEC)
#        self.omega_sid= 2. *  math.pi / 86164.2
#        self.dtau= (result2.rime- result1.time) * self.omega_sid


#        self.lambda_r= math.sqrt( math.pow(self.dX,2) + math.pow(self.dY,2))/ self.dtau 
#        self.ha= -math.atan2( self.dX, self.dY) + dtau/2. + tau

#        self.A= self.lambda_r * math.sin( self.tau) / math.cos( self.lat) 
#        self.k= self.lambda_r * math.cos( self.tau)

class SolverResult():
    """Results of astrometry.net including necessary fits headers"""
    def __init__(self, ra=None, dec=None, jd=None, date_obs=None, lon=None, lat=None):
        self.ra= ra
        self.dec=dec
        self.jd=jd
        self.date_obs=date_obs
        self.lon=lon
        self.lat=lat
    
class SolveField():
    """Solve a field with astrometry.net """
    def __init__(self, fn=None, runTimeConfig=None, logger=None):
        self.fn= fn
        self.runTimeConfig= runTimeConfig
        self.logger = logger
        self.scale  = self.runTimeConfig.cf['ARCSSEC_PER_PIX'] 
        self.radius = self.runTimeConfig.cf['RADIUS']
        self.verbose= self.runTimeConfig.cf['VERBOSE']
        self.replace= self.runTimeConfig.cf['REPLACE']
        self.solver = rts2.astrometry.AstrometryScript(self.fn)
        self.ra= None
        self.dec=None
        self.jd= None
        self.date_obs=None
        self.success= True
        self.blind= False

        try:
            ff=pyfits.open(self.fn,'readonly')
        except:
            self.logger.error('SolveField: file not found {0}'.format( fn)) 
            self.success= False            
        try:
            self.ra=ff[0].header[self.runTimeConfig.cf['ORIRA']]
            self.dec=ff[0].header[self.runTimeConfig.cf['ORIDEC']]
        except KeyError:
            self.logger.error('SolveField: coordinates key error {0} or {1}, solving blindly'.format( 'ORIRA', 'ORIDEC')) 
            self.blind= True

        try:
            self.jd=ff[0].header[self.runTimeConfig.cf['JD']]
            self.date_obs=ff[0].header[self.runTimeConfig.cf['DATE-OBS']]
        except KeyError:
            self.logger.error('SolveField: date time key error {0} or {1}'.format(self.runTimeConfig.cf['JD'], self.runTimeConfig.cf['DATE-OBS'])) 
            self.success=False

        try:
            self.lon=ff[0].header[self.runTimeConfig.cf['SITE-LON']]
            self.lat=ff[0].header[self.runTimeConfig.cf['SITE-LAT']]
        except KeyError:
            self.logger.error('SolveField: site coordinates key error {0} or {1}'.format(self.runTimeConfig.cf['SITE-LON'], self.runTimeConfig.cf['SITE-LAT'])) 
            self.success=False

        ff.close()

    def solveField(self):
        if self.blind:
            center=self.solver.run(scale=self.scale, replace=self.replace,verbose=self.verbose)
        else:
            center=self.solver.run(scale=self.scale,ra=self.ra,dec=self.dec,radius=self.radius,replace=self.replace,verbose=self.verbose)

        if len(center)==2:
            self.logger.debug('SolveField: found center {0}'.format( repr(center)))
            return SolverResult( ra=center[0], dec=center[1], jd=self.jd, date_obs=self.date_obs, lon=self.lon, lat=self.lat)
        else:
            self.logger.debug('SolveField: center not found')
            return None

class MeasurementThread(threading.Thread):
    """Thread receives image path from rts2 and calculates the HA and polar distance of the HA axis intersection as soon as two images are present"""
    def __init__(self, path_q=None, result_q=None, runTimeConfig=None, logger=None):
        super(MeasurementThread, self).__init__()
        self.path_q = path_q
        self.result_q = result_q
        self.runTimeConfig= runTimeConfig
        self.logger= logger
        self.stoprequest = threading.Event()
        self.logger.debug('MeasurementThread: init finished')
        self.results=[]

    def run(self):
        self.logger.debug('MeasurementThread: running')

        while not self.stoprequest.isSet():
            path=None
            try:
                path = self.path_q.get(True, 1.)            
                self.logger.debug('MeasurementThread: next path {0}'.format(path))
            except Queue.Empty:
                continue

            if path:
                if self.runTimeConfig.cf['TEST']:
                    path= self.runTimeConfig.cf['TEST_FIELDS']
                    self.logger.debug('MeasurementThread: test replacement fits: {0}'.format(path))

                sf= SolveField(fn=path, runTimeConfig=self.runTimeConfig, logger=self.logger)
                if sf.success:
                    sr= sf.solveField()
                    if sr:
                        self.results.append(sr)
                        self.result_q.put(self.results)

                        if len(self.results) > 1:
                            kinga=KingA(self.results)
                    else:
                        self.logger.error('MeasurementThread: error within solver (not solving)')
                else:
                    self.logger.error('MeasurementThread: error within solver')


    def join(self, timeout=None):
        self.logger.debug('MeasurementThread: join, timeout {0}'.format(timeout))
        self.stoprequest.set()
        super(MeasurementThread, self).join(timeout)


class AcquireData(rts2pa.PAScript):
    """Create a thread, set the mount, take images"""
    # __init__ of base class is executed

    def takeImages(self, path_q=None, ul=None):
        r2c.setValue('exposure', self.runTimeConfig.cf['EXPOSURE_TIME'])
        r2c.setValue('SHUTTER','LIGHT')

        for img in range(0, ul):
            path = r2c.exposure()
            
            dst= self.environment.moveToRunTimePath(path)
            path_q.put(dst)
            self.logger.debug('takeImages: destination image path {0}'.format(dst))
            #sleep(self.runTimeConfig.cf['SLEEP'])

    def setMount(self):
        self.logger.debug( 'setMount: Starting {0}'.format(self.runTimeConfig.cf['CONFIGURATION_FILE']))
        
        # fetch site latitude
        obs=ephem.Observer()
        obs.lon= str(r2c.getValueFloat('longitude', 'centrald'))
        obs.lat= str(r2c.getValueFloat('latitude', 'centrald'))
        # ha, pd to RA, DEC
        dec= 90. - self.runTimeConfig.cf['PD'] 
        siderealT= obs.sidereal_time() 
        ra=  siderealT - self.runTimeConfig.cf['HA']
        self.logger.debug('longitude: {0}, latitude {1}, sid time {2} ra {3} dec {4}'.format(obs.lon, obs.lat, siderealT, ra, dec))
        # set mount
        r2c.radec( ra, dec)

    def run(self):
        path_q = Queue.Queue()
        result_q = Queue.Queue()
        mt= MeasurementThread( path_q, result_q, self.runTimeConfig, self.logger) 
        mt.start()
        #self.setMount()

        try:
            ul= int(self.runTimeConfig.cf['DURATION']/self.runTimeConfig.cf['SLEEP'])
            self.logger.debug('run: steps {0}'.format(ul))
        except:
            self.logger.debug('run: sleep must not be zero: {0}'.format(self.runTimeConfig.cf['SLEEP']))

        self.takeImages(path_q, ul)

        for i in range (0, ul):
            self.logger.debug('main: {0}'.format(result_q.get()))

        mt.join(1.)


if __name__ == "__main__":

    ac= AcquireData(scriptName=sys.argv[0]).run()
