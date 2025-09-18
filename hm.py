# -*- coding: utf-8 -*-
"""
Created on Mon Sep  8 16:27:11 2025

@author: tabbet_j
"""

### HEALTH  MONITOR


# import time
# import datetime
# from time import strftime, localtime
# import serial
# import glob
# import sys
# #try:
# print('Do not forget to run as sudo')
# ports=glob.glob('/dev/ttyACM*')
# print(ports)
# ser=serial.Serial(ports[0],57600,timeout=10)

# # while True:
# # print(ser.readline().decode('utf-8'))
# # except KeyboardInterrupt:
# # ser.close()


    
# output=[]
# row=[]
# start=datetime.datetime(2025,9,8).timestamp()
# rc=0
# try:
#     while True:
#         t_atr=time.time()
#         f_ind=0
#         while t_atr<start+(3600*24):
#             ftitle = str ( strftime ('%Y -%m -%d', localtime(start)))+".csv"
#             if f_ind==0:
#                 with open(ftitle,'a') as f:
#                     f.write('TSE(s),HTS221_Temp(C),HTS221_RelHum(%),LPS22HH_Temp(C),LPS22HH_Press(kPa),LIS2DW12_AcX(ms-2),LIS2DW12_AcY(ms-2),LIS2DW12_AcZ(ms-2),IIS3DHHC_AcX(ms-2),IIS3DHHC_AcY(ms-2),IIS3DHHC_AcZ(ms-2),LSM6DSOX_AcX(ms-2),LSM6DSOX_AcY(ms-2),LSM6DSOX_AcZ(ms-2),LSM6DSOX_GyroX(dps),LSM6DSOX_GyroY(dps),LSM6DSOX_GyroZ(dps),STTS751_Temp(C),LIS2MDL_MagX(Gauss),LIS2MDL_MagY(Gauss),LIS2MDL_MagZ(Gauss)\n')
#                 f_ind=1
#             a = ser.readline()
#             b=a.decode()
            
#             if "HTS221: Temperature:" in b:
#                 t=float(b[21:-3])
#                 # print(t)

#                 rc+=1
#             if "HTS221: Relative Humidity:" in b:
#                 h=float(b[26:-3])
#                 # print(h)

#                 rc+=1
#             if "LPS22HH: Temperature:" in b:
#                 t2=float(b[21:-3])
#                 # print(t2)

#                 rc+=1
#             if "LPS22HH: Pressure:" in b:
#                 p=float(b[18:-5])
#                 # print(p)

#                 rc+=1
#             if "LIS2DW12: Accel (m.s-2):" in b:
#                 bsub=b.split(':')
#                 acx=float(bsub[3][:-3])
#                 acy=float(bsub[4][:-3])
#                 acz=float(bsub[5])
#                 # print(acx,acy,acz)

#                 rc+=1
#             if "IIS3DHHC: Accel (m.s-2):" in b:
#                 bsub=b.split(':')
#                 acx2=float(bsub[3][:-3])
#                 acy2=float(bsub[4][:-3])
#                 acz2=float(bsub[5])

#                 rc+=1
#                 # print(acx2,acy2,acz2)
#             if "LSM6DSOX: Accel (m.s-2):" in b:
#                 bsub=b.split(':')
#                 acx3=float(bsub[3][:-3])
#                 acy3=float(bsub[4][:-3])
#                 acz3=float(bsub[5])

#                 rc+=1
#                 # print(acx3,acy3,acz3)
#             if "LSM6DSOX: GYro (dps):" in b:
#                 bsub=b.split(':')
#                 gx=float(bsub[3][:-3])
#                 gy=float(bsub[4][:-4])
#                 gz=float(bsub[5])

#                 rc+=1
#                 # print(gx,gy,gz)
#             if "STTS751: Temperature:" in b:
#                 t3=float(b[21:-3])

#                 rc+=1
#                 # print(t3)
#             if "LIS2MDL: Magn (Gauss):" in b:
#                 bsub=b.split(':')
#                 bx=float(bsub[3][:-3])
#                 by=float(bsub[4][:-3])
#                 bz=float(bsub[5])

#                 rc+=1
#                 # print(bx,by,bz)
#             # print('rc',rc)
#             if rc==10:
#                 t_atr=time.time()
#                 out_line=[t_atr,t,h,t2,p,acx,acy,acz,acx2,acy2,acz2,acx3,acy3,acz3,gx,gy,gz,t3,bx,by,bz]
#                 print(t_atr,t,h,t2,p,acx,acy,acz,acx2,acy2,acz2,acx3,acy3,acz3,gx,gy,gz,t3,bx,by,bz)
#                 with open(ftitle,'a') as f:
#                     for i in out_line:
#                         f.write('%s,' %i)
#                     f.write('\n')
#                 rc=0

#         else:
#             start+=(3600*24)
#             row=[]
            

            

# except KeyboardInterrupt:
#     ser.close()
#     #%%
    
import time
import datetime
from time import strftime, localtime
import serial
import glob
import sys
#try:
print('Do not forget to run as sudo')
ports=glob.glob('/dev/ttyACM*')
print(ports)
ser=serial.Serial(ports[0],57600,timeout=10)

output=[]
row=[]
start=datetime.datetime(2025,9,8).timestamp()
rc=0
try:
    while True:
        t_atr=time.time()
        f_ind=0
        while t_atr<start+(3600*24):
            ftitle = str ( strftime ('%Y -%m -%d', localtime(start)))+".csv"
            if f_ind==0:
                with open(ftitle,'a') as f:
                    f.write('TSE,Temp,Humi,Pres,mx,my,mz\n')
                f_ind=1

            a = ser.readline()
            if a.decode()[0:6] == "Temp: ":
                # print("temperature: " + a.decode()[3:])
                try:
                    t = float(a.decode()[6:])  
                except:
                    Exception
                rc+=1
            if a.decode()[0:6] == "Humi: ":
                # print("humidity: " + a.decode()[3:])
                try:
                    h = float(a.decode()[6:])
                except:
                    Exception
                rc+=1
            if a.decode()[0:6] == "Pres: ":
                # print("pressure: " + a.decode()[3:])
                try:
                    p = float(a.decode()[6:])
                except:
                    Exception
                rc+=1
            if a.decode()[0:6] == "Magx: ":
                # print("Mag field x: " + a.decode()[4:])
                try:
                    mx = float(a.decode()[6:])
                except:
                    Exception
                rc+=1
            if a.decode()[0:6] == "Magy: ":
                # print("Mag field y: " + a.decode()[4:])
                try:
                    my = float(a.decode()[6:])  
                except:
                    Exception
                rc+=1
            if a.decode()[0:6] == "Magz: ":
                # print("Mag field z: " + a.decode()[4:])
                try:
                    mz = float(a.decode()[6:])
                except:
                    Exception
                rc+=1
    
                if rc==6:
                    t_atr=time.time()
                    
                    out_line=[t_atr,t,h,p,mx,my,mz]
                    print(t_atr,t,h,p,mx,my,mz)
                    with open(ftitle,'a') as f:
                        for i in out_line:
                            f.write('%s,' %i)
                        f.write('\n')
                    rc=0
    
            else:
                start+=(3600*24)
                row=[]
            

            

except KeyboardInterrupt:
    ser.close()
