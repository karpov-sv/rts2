#!/usr/bin/env python3
# (C) 2016, Markus Wildi, wildi.markus@bluewin.ch
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
#
'''

Determine sky positions for a pointing model, SExtractor, astrometry.net

'''

__author__ = 'wildi.markus@bluewin.ch'

import sys,os
import argparse
import logging
import socket
import numpy as np
import requests
import pyinotify
import importlib

from multiprocessing import Lock, Queue, cpu_count

from datetime import datetime
from astropy import units as u
from astropy.time import Time
from astropy.coordinates import EarthLocation
from astropy.coordinates import SkyCoord,Longitude,Latitude

from astropy.utils import iers
# astropy pre 1.2.1 may not work correctly
#  wget http://maia.usno.navy.mil/ser7/finals2000A.all
# together with IERS_A_FILE
# see Working Offline
#     http://docs.astropy.org/en/stable/utils/iers.html#utils-iers
### iers.conf.auto_download = False  
try:
  iers.IERS.iers_table = iers.IERS_A.open(iers.IERS_A_FILE)
#                                               ###########
except:
  print('download:')
  print('wget http://maia.usno.navy.mil/ser7/finals2000A.all')
  sys.exit(1)

import u_point.sextractor_3 as sextractor_3
from u_point.callback import AnnoteFinder
from u_point.callback import  AnnotatedPlot
from u_point.notify import EventHandler 
from u_point.worker import Worker
from u_point.solver import SolverResult,Solver
from u_point.script import Script
from u_point.refraction import Refraction
try:
  import ref_index.ref_index as ref_index
except:
  # exit only in case it is used
  import_message='ref_index.py not available on local system, see README, exiting'

class Analysis(Script):
  def __init__(
      self, dbg=None,
      lg=None,
      break_after=None,
      base_path=None,
      obs=None,
      acquired_positions=None,
      analyzed_positions=None,
      acq_e_h=None,
      px_scale=None,
      ccd_size=None,
      ccd_angle=None,
      verbose_astrometry=None,
      ds9_display=None,
      solver=None,
      transform=None,
  ):
    Script.__init__(self,lg=lg,break_after=break_after,base_path=base_path,obs=obs,acquired_positions=acquired_positions,analyzed_positions=analyzed_positions,acq_e_h=acq_e_h)
    #
    self.px_scale=px_scale
    self.ccd_size=ccd_size
    self.ccd_angle=args.ccd_angle
    self.verbose_astrometry=verbose_astrometry
    self.ds9_display=ds9_display
    self.solver=solver
    self.transform=transform
    #
    self.ccd_rotation=self.rot(self.ccd_angle)
    self.acquired_positions=acquired_positions
    self.analyzed_positions=analyzed_positions
    self.dt_utc=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')


  def catalog_to_apparent(self,sky=None,pcn=None):
    # use this to check the internal accuracy of astropy
    #mnt_eq=SkyCoord(ra=rw['mnt_ra'],dec=rw['mnt_dc'], unit=(u.rad,u.rad), frame='icrs',obstime=dt_utc,location=self.obs)
      
    if sky.eq_mount:
      tr_t_tf=self.transform.transform_to_hadec
    else:
      tr_t_tf=self.transform.transform_to_altaz
      
    # cat_ll_ap is either cat_aa or cat_ha
    sky.cat_ll_ap=tr_t_tf(tf=sky.cat_ic,sky=sky)
    
  # rotation matrix for xy2lonlat
  def rot(self,rads):
    s=np.sin(rads)
    c=np.cos(rads)
    return np.matrix([[c, -s],[s,  c]])
    
  def xy2lonlat_apparent(self,px=None,py=None,sky=None,pcn=None):
    ln0=sky.cat_ll_ap.ra.radian
    lt0=sky.cat_ll_ap.dec.radian
    
    # ccd angle relative to +dec or +alt
    p=np.array([px,py])
    p_r= self.ccd_rotation.dot(p)               
    px_r=p_r[0,0]
    py_r=p_r[0,1]
    self.lg.debug('{0}:id: {1}, xy2lonlat_apparent: px: {2}, py: {3}, px_r: {4}, py_r: {5}'.format(pcn,sky.nml_id,int(px),int(py),int(px_r),int(py_r)))
    # small angle approximation for inverse gnomonic projection
    # ln0,lt0: field center
    # scale: angle/pixel [radian]
    # px,py from SExtractor are relative to the center equals x,y physical
    # FITS with astrometry:
    #  +x: -ra
    #  +y: +dec
    # HA/Dec, AltAz both left handed coordinate systems
    if sky.eq_mount:
      lon=ln0 + px_r * self.px_scale/np.cos(lt0)
      lat=lt0 + py_r * self.px_scale
      #self.lg.debug('{0}: sextract   center: {1:12.7f} {2:12.7f}'.format(pcn,ln0*180./np.pi,lt0*180./np.pi))
      #self.lg.debug('{0}: sextract   star  : {1:12.7f} {2:12.7f}'.format(pcn,lon*180./np.pi,lat*180./np.pi))
    else:
      # astropy AltAz frame is right handed
      # +x: +az
      # +y: +alt
      lon=ln0 + px_r * self.px_scale/np.cos(lt0)
      lat=lt0 + py_r * self.px_scale
      
    ptfn=self.expand_base_path(fn=sky.image_fn)
    self.lg.debug('{0}:id: {1}    sextract   result: {2:12.7f} {3:12.7f}, file: {4}'.format(pcn,sky.nml_id,lon*180./np.pi,lat*180./np.pi,ptfn))
    if sky.eq_mount:
      # apparent "plus corrections"
      sky.mnt_ll_sxtr=SkyCoord(ra=lon,dec=lat, unit=(u.radian,u.radian), frame='gcrs',location=self.obs)
    else:
      sky.mnt_ll_sxtr=SkyCoord(az=lon,alt=lat, unit=(u.radian,u.radian), frame='altaz',location=self.obs)
  
  def sextract(self,sky=None,pcn='single'):
    #  self.lg.debug('sextract: Yale catalog number: {}'.format(int(sky.cat_no)))
    sx = sextractor_3.Sextractor(['EXT_NUMBER','X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'],sexpath='/usr/bin/sextractor',sexconfig='/usr/share/sextractor/default.sex',starnnw='/usr/share/sextractor/default.nnw')
    ptfn=self.expand_base_path(fn=sky.image_fn)
    try:
      sx.runSExtractor(filename=ptfn)
    except Exception as e:
      self.lg.error('exception: {}'.format(e))
      return None,None
    if len(sx.objects)==0:
      return None,None
    
    sx.sortObjects(sx.get_field('MAG_BEST'))
    try:
      brst = sx.objects[0]
      self.lg.warn('{0}:id: {1}, number of sextract objects: {2}, fn: {3} '.format(pcn,sky.nml_id,len(sx.objects),ptfn))
    except:
      self.lg.warn('{0}:id: {1}, no sextract result for: {2} '.format(pcn,sky.nml_id,ptfn))
      return None,None
    i_x = sx.get_field('X_IMAGE')
    i_y = sx.get_field('Y_IMAGE')
    i_m = sx.get_field('MAG_BEST')
    i_f = sx.get_field('FLAGS')
    # relative to the image center
    # Attention: AltAz of in x
    x=brst[i_x]-self.ccd_size[0]/2.
    y=brst[i_y]-self.ccd_size[1]/2.
    self.lg.debug('{0}:id: {1},   sextract relative to center: {2:4.1f} px,{3:4.1f} px,{4:4.3f} mag, SX flag: {5}'.format(pcn,sky.nml_id,x,y,brst[i_m],brst[i_f]))
    #ToDo prov.
    if self.ds9_display:
      from pyds9 import DS9
      display = DS9()
      display.set('file {0}'.format(ptfn))
      display.set('regions', 'image; circle {0} {1} 10'.format(brst[i_x],brst[i_y]))

    return x,y
    
  def astrometry(self,sky=None,pcn=None):
    if self.solver is None:
      return

    ptfn=self.expand_base_path(fn=sky.image_fn)

    self.lg.debug('{0}:id: {1},           mount set: {2:12.7f} {3:12.7f}, file: {4}'.format(pcn,sky.nml_id,sky.cat_ic.ra.degree,sky.cat_ic.dec.degree,ptfn))
    if sky.mnt_ra_rdb.ra.radian != 0. and sky.mnt_ra_rdb.dec.radian != 0.:
      self.lg.debug('{0}:id: {1},     mount read back: {2:12.7f} {3:12.7f}, file: {4}'.format(pcn,sky.nml_id,sky.mnt_ra_rdb.ra.degree,sky.mnt_ra_rdb.dec.degree,ptfn))

    sr= self.solver.solve_field(fn=ptfn,ra=sky.cat_ic.ra.degree,dec=sky.cat_ic.dec.degree,)
    if sr is not None:
      self.lg.debug('{0}:id: {1},ic astrometry result: {2:12.7f} {3:12.7f}, file: {4}'.format(pcn,sky.nml_id,sr.ra,sr.dec,ptfn))
      # astrometry.net returns ICRS coordinates, not GCRS
      mnt_ll=SkyCoord(ra=sr.ra,dec=sr.dec, unit=(u.degree,u.degree), frame='icrs',location=self.obs,obstime=sky.dt_end)
      if sky.eq_mount:
        tr_t_tf=self.transform.transform_to_hadec
      else:
        tr_t_tf=self.transform.transform_to_altaz
        
      # astrometry.net returns ICRS coordinates, ICRS to apparent, including refraction
      sky.mnt_ll_astr=tr_t_tf(tf=mnt_ll,sky=sky)

      if sky.eq_mount:
        self.lg.debug('{0}:id: {1},ha astrometry result: {2:12.7f} {3:12.7f}, file: {4}'.format(pcn,sky.nml_id,sky.mnt_ll_astr.ra.degree,sky.mnt_ll_astr.dec.degree,ptfn))
      else:
        self.lg.debug('{0}:id: {1},aa astrometry result: {2:12.7f} {3:12.7f}, file: {4}'.format(pcn,sky.nml_id,sr.ra,sr.dec,ptfn))
    
    else:
      self.lg.debug('{0}:id: {1},no astrometry result: file: {2}'.format(pcn,sky.nml_id,ptfn))

  def re_plot(self,i=0,animate=None):
    self.lg.debug('re_plot: reploting')
    self.fetch_positions(sys_exit=True,analyzed=False)
    self.fetch_positions(sys_exit=False,analyzed=True)
    self.lg.debug('re_plot: positions fetched')
    #                                                               if self.anl=[]
    # sxtr is RA,Dec to compare with astr
    # ToDo think about tranforming astr to AltAz,pressure=0.
    # use SphericalRepr..
    # ToDo [0] ugly
    if self.sky_acq[0].eq_mount:
      mnt_ll_sxtr_lon=[x.mnt_ll_sxtr.ra.degree for i,x in enumerate(self.sky_anl) if x is not None and x.mnt_ll_sxtr is not None]
      mnt_ll_sxtr_lat=[x.mnt_ll_sxtr.dec.degree for x in self.sky_anl if x is not None and x.mnt_ll_sxtr is not None]
    else:
      mnt_ll_sxtr_lon=[x.mnt_ll_sxtr.az.degree for i,x in enumerate(self.sky_anl) if x is not None and x.mnt_ll_sxtr is not None]
      mnt_ll_sxtr_lat=[x.mnt_ll_sxtr.alt.degree for x in self.sky_anl if x is not None and x.mnt_ll_sxtr is not None]
      
    if self.sky_acq[0].eq_mount:
      mnt_ll_astr_lon=[x.mnt_ll_astr.ra.degree for i,x in enumerate(self.sky_anl) if x is not None and x.mnt_ll_astr is not None]  
      mnt_ll_astr_lat=[x.mnt_ll_astr.dec.degree for x in self.sky_anl if x is not None and x.mnt_ll_astr is not None]
    else:
      mnt_ll_astr_lon=[x.mnt_ll_astr.az.degree for i,x in enumerate(self.sky_anl) if x is not None and x.mnt_ll_astr is not None]  
      mnt_ll_astr_lat=[x.mnt_ll_astr.alt.degree for x in self.sky_anl if x is not None and x.mnt_ll_astr is not None]

    # attention: ax.clear deletes annotations too
    self.ax.clear()
    self.ax.scatter(self.cat_ll_ap_lon, self.cat_ll_ap_lat,color='blue',s=120.)
    self.ax.scatter(mnt_ll_sxtr_lon, mnt_ll_sxtr_lat,color='red',s=40.)
    self.ax.scatter(mnt_ll_astr_lon, mnt_ll_astr_lat,color='yellow',s=10.)
    # mark last positions
    if len(mnt_ll_sxtr_lon) > 0:
      self.ax.scatter(mnt_ll_sxtr_lon[-1],mnt_ll_sxtr_lat[-1],color='green',facecolors='none', edgecolors='green',s=300.)
    if len(mnt_ll_astr_lon) > 0:
      self.ax.scatter(mnt_ll_astr_lon[-1],mnt_ll_astr_lat[-1],color='green',facecolors='none', edgecolors='magenta',s=400.)

    #self.ax.set_xlim([0.,360.]) 

    ttl_frg='azimuth'
    if self.sky_acq[0].eq_mount:
      ttl_frg='HA'

    if animate:
      self.ax.set_title(self.title, fontsize=10)
      now=str(Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date'))[:-7]
      self.ax.set_xlabel('{0} [deg], at: {1} [UTC]'.format(ttl_frg,now))
    else:
      self.ax.set_title(self.title)
      self.ax.set_xlabel('{0} [deg]'.format(ttl_frg))
    
    self.ax.annotate('positions:',color='black', xy=(0.03, 0.05), xycoords='axes fraction')
    self.ax.annotate('acquired',color='blue', xy=(0.16, 0.05), xycoords='axes fraction')
    self.ax.annotate('sxtr',color='red', xy=(0.30, 0.05), xycoords='axes fraction')
    self.ax.annotate('astr: yellow',color='black', xy=(0.43, 0.05), xycoords='axes fraction')
    self.ax.annotate('last sxtr',color='green', xy=(0.63, 0.05), xycoords='axes fraction')
    self.ax.annotate('last astr',color='magenta', xy=(0.83, 0.05), xycoords='axes fraction')

    ttl_frg='altitude'
    if self.sky_acq[0].eq_mount:
      ttl_frg='declination'
    self.ax.set_ylabel('{0} [deg]'.format(ttl_frg))
                       
    self.ax.grid(True)
    # ToDo: was nu? debug?
    #annotes=['{0:.1f},{1:.1f}: {2}'.format(x.cat_ic.ra.degree,x.cat_ic.dec.degree,x.image_fn) for x in self.sky_acq]
    # ToDo: use AltAz, HA coordinates
    
    if self.sky_acq[0].eq_mount:
      annotes=['{0:.1f},{1:.1f}: {2}'.format(x.cat_ll_ap.ra.degree,x.cat_ll_ap.dec.degree,x.image_fn) for x in self.sky_acq]
    else:
      annotes=['{0:.1f},{1:.1f}: {2}'.format(x.cat_ll_ap.az.degree,x.cat_ll_ap.alt.degree,x.image_fn) for x in self.sky_acq]

    nml_ids=[x.nml_id for x in self.sky_acq if x.mnt_aa_rdb is not None]
    aps=[AnnotatedPlot(xx=self.ax,nml_id=nml_ids,lon=self.cat_ll_ap_lon,lat=self.cat_ll_ap_lat,annotes=annotes)]

    self.lg.debug('re_plot: end')
    try:
      self.af.aps=aps
      return
    except AttributeError:
      return aps
    
  def plot(self,title=None,animate=None,delete=None):
    import matplotlib
    import matplotlib.animation as animation
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    self.ax = fig.add_subplot(111)#, projection="mollweide")
    self.title=title
    # we want to see something, values are only for the plot
    # this is cat not apparent

    self.lg.debug('plot: fetching catalog positions from file')
    self.fetch_positions(sys_exit=True,analyzed=False)
    self.lg.debug('plot: transforming catalog to apparent positions')
    if self.sky_acq[0].eq_mount:
      self.cat_ll_ap_lon=[self.transform.transform_to_hadec(tf=x.cat_ic,sky=x).ra.degree for x in self.sky_acq]
      self.cat_ll_ap_lat=[self.transform.transform_to_hadec(tf=x.cat_ic,sky=x).dec.degree for x in self.sky_acq]
      # ToDo not faster:
      # May be: http://chriskiehl.com/article/parallelism-in-one-line/
      #self.cat_ll_ap_lon=list()
      #self.cat_ll_ap_lat=list()
      #for x in self.sky_acq:
      #  y=self.transform.transform_to_hadec(tf=x.cat_ic,sky=x)
      #  self.cat_ll_ap_lon.append(y.ra.degree)
      #  self.cat_ll_ap_lat.append(y.dec.degree)
    else:
      #cat_ll_ap_lat,cat_ll_ap_lat=[(self.to_altaz(ic=x.cat_ic).az.degree,self.to_altaz(ic=x.cat_ic).alt.degree) for x in self.sky_acq]
      self.cat_ll_ap_lon=[self.to_altaz(ic=x.cat_ic,sky=x).az.degree for x in self.sky_acq]
      self.cat_ll_ap_lat=[self.to_altaz(ic=x.cat_ic,sky=x).alt.degree for x in self.sky_acq]
    self.lg.debug('plot: catalog apparent positions fetched')
    if animate: #                                     do not remove ","
      ani = animation.FuncAnimation(fig, self.re_plot, fargs=(animate,),interval=2000)
      self.lg.debug('plot: plot animation started')

    self.lg.debug('plot: building plots')
    aps=self.re_plot(animate=animate)
    # analyzed=False means: delete a position only in acquired 
    self.af = AnnoteFinder(ax=self.ax,aps=aps,xtol=5., ytol=5.,ds9_display=self.ds9_display,lg=self.lg,annotate_fn=True,analyzed=True,delete_one=self.delete_one_position)
    fig.canvas.mpl_connect('button_press_event',self.af.mouse_event)
    if delete:
      fig.canvas.mpl_connect('key_press_event',self.af.keyboard_event)
    self.lg.debug('plot: annotations created')

    #plt.show(block=False)
    plt.show()
    return 
    
# really ugly!
def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)

if __name__ == "__main__":

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Analyze observed positions')
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')

  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--acquired-positions', dest='acquired_positions', action='store', default='acquired_positions.acq', help=': %(default)s, already observed positions')
  parser.add_argument('--base-path', dest='base_path', action='store', default='/tmp/u_point/',type=str, help=': %(default)s , directory where images are stored')
  parser.add_argument('--analyzed-positions', dest='analyzed_positions', action='store', default='analyzed_positions.anl', help=': %(default)s, already observed positions')
  # group plot
  parser.add_argument('--plot', dest='plot', action='store_true', default=False, help=': %(default)s, plot results')
  parser.add_argument('--ds9-display', dest='ds9_display', action='store_true', default=False, help=': %(default)s, inspect image and region with ds9')
  parser.add_argument('--animate', dest='animate', action='store_true', default=False, help=': %(default)s, True: plot will be updated whil acquisition is in progress')
  parser.add_argument('--delete', dest='delete', action='store_true', default=False, help=': %(default)s, True: click on data point followed by keyboard <Delete> deletes selected acquired measurements from file --acquired-positions')
  parser.add_argument('--pos-by-position', dest='pos_by_position', action='store_true', default=False, help=': %(default)s, no multiprocessing, single worker mode with display of each measurement')

  #
  parser.add_argument('--pixel-scale', dest='pixel_scale', action='store', default=1.7,type=float, help=': %(default)s [arcsec/pixel], arcmin/pixel of the CCD camera')
  parser.add_argument('--ccd-size', dest='ccd_size', default=[862.,655.],nargs='+', type=float, help=': %(default)s [px], ccd pixel size x,y[px], format p1 p2')
  # angle is defined relative to the positive dec or alt direction
  # the rotation is anti clock wise (right hand coordinate system)
  parser.add_argument('--ccd-angle', dest='ccd_angle', default=0., type=float, help=': %(default)s [deg], ccd angle measured anti clock wise relative to positive Alt or Dec axis, rotation of 180. ')

  # group SExtractor, astrometry.net
  parser.add_argument('--timeout', dest='timeout', action='store', default=120,type=int, help=': %(default)s [sec], astrometry timeout for finding a solution')
  parser.add_argument('--radius', dest='radius', action='store', default=1.,type=float, help=': %(default)s [deg], astrometry search radius')
  parser.add_argument('--do-not-use-astrometry', dest='do_not_use_astrometry', action='store_true', default=False, help=': %(default)s, use astrometry')
  parser.add_argument('--verbose-astrometry', dest='verbose_astrometry', action='store_true', default=False, help=': %(default)s, use astrometry in verbose mode')
  # transforms, coordinates
  parser.add_argument('--transform-class', dest='transform_class', action='store', default='u_astropy', help=': %(default)s, one of (u_sofa|u_astropy|u_libnova|u_pyephem)')
  # see Ronald C. Stone, Publications of the Astronomical Society of the Pacific 108: 1051-1058, 1996 November
  parser.add_argument('--refraction-method', dest='refraction_method', action='store', default='built_in', help=': %(default)s, one of (bennett|saemundsson|stone), see refraction.py')
  parser.add_argument('--refractive-index-method', dest='refractive_index_method', action='store', default='owens', help=': %(default)s, one of (owens|ciddor|edlen) if --refraction-method stone is specified, see refraction.py')

  args=parser.parse_args()
  
  if args.toconsole:
    args.level='DEBUG'

  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)

  pth, fn = os.path.split(sys.argv[0])
  filename=os.path.join(args.base_path,'{}.log'.format(fn.replace('.py',''))) # ToDo datetime, name of the script
  logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
  logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
  logger=logging.getLogger()
    
  if args.toconsole:
    # http://www.mglerner.com/blog/?p=8
    soh=logging.StreamHandler(sys.stdout)
    soh.setLevel(args.level)
    logger.addHandler(soh)
    
  if args.delete:
    args.animate=True
    
  px_scale=args.pixel_scale/3600./180.*np.pi
  solver=None
  if not args.do_not_use_astrometry: # double neg
    solver= Solver(lg=logger,blind=False,scale=px_scale,radius=args.radius,replace=False,verbose=args.verbose_astrometry,timeout=args.timeout)

  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)
    
  # ToDo: not yet ready for prime time 
  acq_e_h=None
  #if args.delete:
  #  # ToDo in case directory is not there
  #  #try:
  #  wm=pyinotify.WatchManager()
  #  wm.add_watch(args.base_path,pyinotify.ALL_EVENTS, rec=True)
  #  acq_e_h=EventHandler(lg=logger,fn=args.acquired_positions)
  #  nt=pyinotify.ThreadedNotifier(wm,acq_e_h)
  #  nt.start()
  #  #except Exception as e:
  #  #  logger.error('directory: {}, das not exist, error: {}'.format(args.base_path,e))
  #  #  sys.exit(1)

  obs=EarthLocation(lon=float(args.obs_lng)*u.degree, lat=float(args.obs_lat)*u.degree, height=float(args.obs_height)*u.m)

  rf_m=None
  ri_m=None
  if 'built_in' not in args.refraction_method:
    if 'ciddor' in  args.refractive_index_method or 'edlen' in  args.refractive_index_method:
      if import_message is not None:
        self.lg.error('u_analyze: {}'.format(import_message))
        sys.exit(1)

    # refraction index method is set within Refraction
    rf=Refraction(lg=logger,obs=obs,refraction_method=args.refraction_method,refractive_index_method=args.refractive_index_method)
    rf_m=getattr(rf, 'refraction_'+args.refraction_method)
    logger.info('refraction method loaded: {}'.format(args.refraction_method))
    
  tf = importlib.import_module('transform.'+args.transform_class)
  logger.info('transformation loaded: {}'.format(args.transform_class))

  transform=tf.Transformation(lg=logger,obs=obs,refraction_method=rf_m)

  anl= Analysis(
    lg=logger,
    obs=obs,
    px_scale=px_scale,
    ccd_size=args.ccd_size,
    ccd_angle=args.ccd_angle/180. * np.pi,
    acquired_positions=args.acquired_positions,
    analyzed_positions=args.analyzed_positions,
    break_after=args.break_after,
    ds9_display=args.ds9_display,
    base_path=args.base_path,
    solver=solver,
    acq_e_h=acq_e_h,
    transform=transform,
  )
  logger.info('analysis object created')

  if args.plot and not args.pos_by_position:
    title='progress: analyzed positions'
    if args.ds9_display:
      title += ':\n click on blue dots to watch image (DS9)'
    if args.delete:
      title += '\n then press <Delete> to remove from the list of acquired positions'

    logger.info('creating plots')
    anl.plot(title=title,animate=args.animate,delete=args.delete)
    sys.exit(0)

  anl.fetch_positions(sys_exit=True,analyzed=False)
  anl.fetch_positions(sys_exit=False,analyzed=True)

  # ToDo not the ideal place
  for o in anl.sky_acq: # this is the origin
    if 'no_transform' in o.transform_name:
      o.transform_name=transform.name
    elif o.transform_name in transform.name:
      pass
    else:
      logger.error('u_analyze: can not mix transformations: {} (from anl file), {}, exiting'.format(o.transform_name,transform.name))
      sys.exit(1)
  
    if 'no_refraction' in o.refraction_method:
      o.refraction_method=args.refraction_method
    elif o.refraction_method in args.refraction_method:
      continue
    else:
      logger.error('u_analyze: can not mix refraction methods: {} (from anl file), {}, exiting'.format(o.transform_name,transform.name))
      sys.exit(1)

  sxtr_analyzed=[x.nml_id for x in anl.sky_anl if x.mnt_ll_sxtr is not None]
  astr_analyzed=[x.nml_id for x in anl.sky_anl if x.mnt_ll_astr is not None]

  lock=Lock()
  work_queue=Queue()  
  for i,o in enumerate(anl.sky_acq):
    if args.do_not_use_astrometry: # double neg
      if o.nml_id in sxtr_analyzed:
        #logger.debug('skiping analyzed position: {}'.format(o.image_fn))
        continue
      else:
        pass
        # logger.debug('adding position: {},{}'.format(i,o.image_fn))
    else:
      if o.nml_id in sxtr_analyzed and o.nml_id in astr_analyzed:
        continue
      else:
        pass
        # logger.debug('adding position: {}, {}'.format(i,o.image_fn))
    
    work_queue.put(o)
    
  if len(anl.sky_anl) and args.ds9_display:
    logger.warn('deleted positions will appear again, these are deliberately not stored, file: {}'.format(args.analyzed_positions))

  cpus=max(1,int(cpu_count())-1) # one left for the user :-))  
  cmd_queue=None    
  next_queue=None    
  if args.pos_by_position:
    cmd_queue=Queue()  
    next_queue=Queue()
    cpus=2 # one worker
    
  processes = list()
  
  for w in range(0,cpus,1):
    p=Worker(work_queue=work_queue,cmd_queue=cmd_queue,next_queue=next_queue,lock=lock,lg=logger,anl=anl)
    logger.debug('starting process: {}'.format(p.name))
    p.start()
    processes.append(p)
    work_queue.put('STOP')

  logger.debug('number of processes started: {}'.format(len(processes)))

  if args.pos_by_position:
    cmd_queue.put('c')
    while True:
      next_queue.get()
      y = input('<RETURN> for next, \'Delete\' for delete last, \'q\' for quit\n')
      print('key: {0}'.format(":".join("{:02x}".format(ord(c)) for c in y)))
      if 'q' in y:
        for p in processes:
          cmd_queue.put('q')
          p.shutdown()
        break
      # ToDo ugly
      elif '1b:5b:33:7e' in ":".join("{:02x}".format(ord(c)) for c in y): # Delete
        cmd_queue.put('dl')
      else:
        cmd_queue.put('c')
  
  for p in processes:
    logger.debug('waiting for process: {} to join'.format(p.name))
    p.join()
                                                            
  logger.debug('DONE')
  sys.exit(0)
