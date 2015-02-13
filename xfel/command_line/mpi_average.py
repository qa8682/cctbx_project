from __future__ import division
#-*- Mode: Python; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 8 -*-
#
# LIBTBX_SET_DISPATCHER_NAME cxi.mpi_average
#

from psana import *
import numpy as np
from xfel.cxi.cspad_ana import cspad_tbx, parse_calib
import libtbx
from libtbx import easy_pickle
import libtbx.option_parser
from scitbx.array_family import flex
import sys
from libtbx.utils import Sorry

def average(argv=None):
  if argv == None:
    argv = sys.argv[1:]

  try:
    from mpi4py import MPI
  except ImportError:
    raise Sorry("MPI not found")

  command_line = (libtbx.option_parser.option_parser(
    usage="""
%s [-p] -c config -x experiment -a address -r run -d detz_offset [-o outputdir] [-A averagepath] [-S stddevpath] [-M maxpath] [-n numevents] [-s skipnevents] [-v]

To write image pickles use -p, otherwise the program writes CSPAD CBFs.
Writing CBFs requires the geometry to be already deployed.

Examples:
cxi.mpi_average -c cxi49812/average.cfg -x cxi49812 -a CxiDs1.0:Cspad.0 -r 25 -d 571

Use one process on the current node to process all the events from run 25 of
experiment cxi49812, using a detz_offset of 571.

mpirun -n 16 cxi.mpi_average -c cxi49812/average.cfg -x cxi49812 -a CxiDs1.0:Cspad.0 -r 25 -d 571

As above, using 16 cores on the current node.

bsub -a mympi -n 100 -o average.out -q psanaq cxi.mpi_average -c cxi49812/average.cfg -x cxi49812 -a CxiDs1.0:Cspad.0 -r 25 -d 571 -o cxi49812

As above, using the psanaq and 100 cores, putting the log in average.out and
the output images in the folder cxi49812.
""" % libtbx.env.dispatcher_name)
                .option(None, "--as_pickle", "-p",
                        action="store_true",
                        default=False,
                        dest="as_pickle",
                        help="Write results as image pickle files instead of cbf files")
                .option(None, "--config", "-c",
                        type="string",
                        default=None,
                        dest="config",
                        metavar="PATH",
                        help="psana config file")
                .option(None, "--experiment", "-x",
                        type="string",
                        default=None,
                        dest="experiment",
                        help="experiment name (eg cxi84914)")
                .option(None, "--run", "-r",
                        type="int",
                        default=None,
                        dest="run",
                        help="run number")
                .option(None, "--address", "-a",
                        type="string",
                        default="CxiDs2.0:Cspad.0",
                        dest="address",
                        help="detector address name (eg CxiDs2.0:Cspad.0)")
                .option(None, "--detz_offset", "-d",
                        type="int",
                        default=None,
                        dest="detz_offset",
                        help="offset (in mm) from sample interaction region to back of CSPAD detector rail (CXI), or detector distance (XPP)")
                .option(None, "--outputdir", "-o",
                        type="string",
                        default=".",
                        dest="outputdir",
                        metavar="PATH",
                        help="Optional path to output directory for output files")
                .option(None, "--averagebase", "-A",
                        type="string",
                        default="{experiment!l}_avg-r{run:04d}",
                        dest="averagepath",
                        metavar="PATH",
                        help="Path to output average image without extension. String substitution allowed")
                .option(None, "--stddevbase", "-S",
                        type="string",
                        default="{experiment!l}_stddev-r{run:04d}",
                        dest="stddevpath",
                        metavar="PATH",
                        help="Path to output standard deviation image without extension. String substitution allowed")
                .option(None, "--maxbase", "-M",
                        type="string",
                        default="{experiment!l}_max-r{run:04d}",
                        dest="maxpath",
                        metavar="PATH",
                        help="Path to output maximum projection image without extension. String substitution allowed")
                .option(None, "--numevents", "-n",
                        type="int",
                        default=None,
                        dest="numevents",
                        help="Maximum number of events to process. Default: all")
                .option(None, "--skipevents", "-s",
                        type="int",
                        default=0,
                        dest="skipevents",
                        help="Number of events in the beginning of the run to skip. Default: 0")
                .option(None, "--verbose", "-v",
                        action="store_true",
                        default=False,
                        dest="verbose",
                        help="Print more information about progress")
                ).process(args=argv)


  if len(command_line.args) > 0 or \
      command_line.options.as_pickle is None or \
      command_line.options.config is None or \
      command_line.options.experiment is None or \
      command_line.options.run is None or \
      command_line.options.address is None or \
      command_line.options.detz_offset is None or \
      command_line.options.averagepath is None or \
      command_line.options.stddevpath is None or \
      command_line.options.maxpath is None:
    command_line.parser.show_help()
    return

  # set this to sys.maxint to analyze all events
  if command_line.options.numevents is None:
    maxevents = sys.maxint
  else:
    maxevents = command_line.options.numevents

  comm = MPI.COMM_WORLD
  rank = comm.Get_rank()
  size = comm.Get_size()

  setConfigFile(command_line.options.config)
  dataset_name = "exp=%s:run=%d:idx"%(command_line.options.experiment, command_line.options.run)
  ds = DataSource(dataset_name)
  address = command_line.options.address
  src = Source('DetInfo(%s)'%address)

  nevent = np.array([0.])

  for run in ds.runs():
    runnumber = run.run()
    # list of all events
    if command_line.options.skipevents > 0:
      print "Skipping first %d events"%command_line.options.skipevents

    times = run.times()[command_line.options.skipevents:]
    nevents = min(len(times),maxevents)
    # chop the list into pieces, depending on rank.  This assigns each process
    # events such that the get every Nth event where N is the number of processes
    mytimes = [times[i] for i in xrange(nevents) if (i+rank)%size == 0]
    for i in xrange(len(mytimes)):
      if i%10==0: print 'Rank',rank,'processing event',rank*len(mytimes)+i,', ',i,'of',len(mytimes)
      evt = run.event(mytimes[i])
      #print "Event #",rank*mylength+i," has id:",evt.get(EventId)
      data = evt.get(ndarray_float64_3, src, 'image0')
      if data is None:
        print "No data"
        continue

      d = cspad_tbx.env_distance(address, run.env(), command_line.options.detz_offset)
      if d is None:
        print "No distance, skipping shot"
        continue
      if 'distance' in locals():
        distance += d
      else:
        distance = np.array([d])

      w = cspad_tbx.evt_wavelength(evt)
      if w is None:
        print "No wavelength, skipping shot"
        continue
      if 'wavelength' in locals():
        wavelength += w
      else:
        wavelength = np.array([w])

      t = cspad_tbx.evt_time(evt)
      if t is None:
        print "No timestamp, skipping shot"
        continue
      if 'timestamp' in locals():
        timestamp += t[0] + (t[1]/1000)
      else:
        timestamp = np.array([t[0] + (t[1]/1000)])

      if 'sum' in locals():
        sum+=data
      else:
        sum=np.array(data, copy=True)
      if 'sumsq' in locals():
        sumsq+=data*data
      else:
        sumsq=data*data
      if 'maximum' in locals():
        maximum=np.maximum(maximum,data)
      else:
        maximum=np.array(data, copy=True)

      nevent += 1

  #sum the images across mpi cores
  if size > 1:
    print "Synchronizing rank", rank
  totevent = np.zeros(nevent.shape)
  comm.Reduce(nevent,totevent)

  if rank == 0 and totevent[0] == 0:
    raise Sorry("No events found in the run")

  sumall = np.zeros(sum.shape)
  comm.Reduce(sum,sumall)

  sumsqall = np.zeros(sumsq.shape)
  comm.Reduce(sumsq,sumsqall)

  maxall = np.zeros(maximum.shape)
  comm.Reduce(maximum,maxall, op=MPI.MAX)

  waveall = np.zeros(wavelength.shape)
  comm.Reduce(wavelength,waveall)

  distall = np.zeros(distance.shape)
  comm.Reduce(distance,distall)

  timeall = np.zeros(timestamp.shape)
  comm.Reduce(timestamp,timeall)

  if rank==0:
    if size > 1:
      print "Synchronized"

    # Accumulating floating-point numbers introduces errors,
    # which may cause negative variances.  Since a two-pass
    # approach is unacceptable, the standard deviation is
    # clamped at zero.
    mean = sumall / float(totevent[0])
    variance = (sumsqall / float(totevent[0])) - (mean**2)
    variance[variance < 0] = 0
    stddev = np.sqrt(variance)

    wavelength = waveall[0] / totevent[0]
    distance = distall[0] / totevent[0]
    pixel_size = cspad_tbx.pixel_size
    saturated_value = cspad_tbx.dynamic_range
    timestamp = timeall[0] / totevent[0]
    timestamp = (int(timestamp), timestamp % int(timestamp) * 1000)
    timestamp = cspad_tbx.evt_timestamp(timestamp)

    if command_line.options.as_pickle:
      extension = ".pickle"
    else:
      extension = ".cbf"

    dest_paths = [cspad_tbx.pathsubst(command_line.options.averagepath + extension, evt, ds.env()),
                  cspad_tbx.pathsubst(command_line.options.stddevpath  + extension, evt, ds.env()),
                  cspad_tbx.pathsubst(command_line.options.maxpath     + extension, evt, ds.env())]
    dest_paths = [os.path.join(command_line.options.outputdir, path) for path in dest_paths]

    if command_line.options.as_pickle:
      sections = parse_calib.calib2sections(libtbx.env.find_in_repositories("xfel/metrology/CSPad/run4/CxiDs1.0_Cspad.0"))
      beam_center, active_areas = cspad_tbx.cbcaa(
        cspad_tbx.getConfig(address, ds.env()), sections)

      class fake_quad(object):
        def __init__(self, q, d):
          self.q = q
          self.d = d

        def quad(self):
          return self.q

        def data(self):
          return self.d

      quads = [fake_quad(i, mean[i*8:(i+1)*8,:,:]) for i in xrange(4)]
      mean = cspad_tbx.CsPadDetector(
        address, evt, ds.env(), sections, quads=quads)
      mean = flex.double(mean.astype(np.float64))

      quads = [fake_quad(i, stddev[i*8:(i+1)*8,:,:]) for i in xrange(4)]
      stddev = cspad_tbx.CsPadDetector(
        address, evt, ds.env(), sections, quads=quads)
      stddev = flex.double(stddev.astype(np.float64))

      quads = [fake_quad(i, maxall[i*8:(i+1)*8,:,:]) for i in xrange(4)]
      maxall = cspad_tbx.CsPadDetector(
        address, evt, ds.env(), sections, quads=quads)
      maxall = flex.double(maxall.astype(np.float64))

      split_address = cspad_tbx.address_split(address)
      old_style_address = split_address[0] + "-" + split_address[1] + "|" + split_address[2] + "-" + split_address[3]

      for data, path in zip([mean, stddev, maxall], dest_paths):
        print "Saving", path

        d = cspad_tbx.dpack(
          active_areas=active_areas,
          address=old_style_address,
          beam_center_x=pixel_size * beam_center[0],
          beam_center_y=pixel_size * beam_center[1],
          data=data,
          distance=distance,
          pixel_size=pixel_size,
          saturated_value=saturated_value,
          timestamp=timestamp,
          wavelength=wavelength)

        easy_pickle.dump(path, d)
    else:
      # load a header only cspad cbf from the slac metrology
      from xfel.cftbx.detector import cspad_cbf_tbx
      import pycbf
      base_dxtbx = cspad_cbf_tbx.env_dxtbx_from_slac_metrology(run.env(), src)
      if base_dxtbx is None:
        raise Sorry("Couldn't load calibration file for run %d"%run.run())

      for data, path in zip([mean, stddev, maxall], dest_paths):
        print "Saving", path

        cspad_img = cspad_cbf_tbx.format_object_from_data(base_dxtbx, data, distance, wavelength, timestamp, address)
        cspad_img._cbf_handle.write_widefile(path, pycbf.CBF,\
          pycbf.MIME_HEADERS|pycbf.MSG_DIGEST|pycbf.PAD_4K, 0)


if (__name__ == "__main__"):
  sys.exit(average(sys.argv[1:]))
