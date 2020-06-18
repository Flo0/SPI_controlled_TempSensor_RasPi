#!/usr/bin/python3
# -*- coding: utf-8 -*-
'''
     Temperatursensor Simulation
     W. Tasin - 2020
'''
import sys, os, fcntl
from PyQt5.QtWidgets import QWidget, QApplication, QMainWindow, \
                             QVBoxLayout, QDial, QLabel

class MyMainWindow(QMainWindow):
    def __init__(self, ch=0, parent=None, name="Temperatur Simulation"):
        super().__init__(parent)
        self.setWindowTitle(name)
        self.__channel=ch
        
        cw=QWidget(self)
        layout=QVBoxLayout(cw)
        
        self.setCentralWidget(cw)
        self.__label=QLabel("Temperatur:        ", self)
        self.__dial=QDial(self)
        self.__dial.setRange(100, 400)
        self.__dial.setSingleStep(1)
        self.__dial.setPageStep(10)
        self.__dial.setTracking(False)

        layout.addWidget(QLabel("Channel: {}".format(ch), self))
        layout.addWidget(self.__label)
        layout.addWidget(self.__dial)
        self.__dial.valueChanged.connect(self.__sliderChanged)
        self.__dial.setValue(200)
        self.__sliderChanged()
        
    def __sliderChanged(self):
        temp=self.__dial.value()/10
        self.__label.setText("Temperatur: {:.1f}°".format(temp))
        temp=temp*4096.0/330
        with open('/tmp/wiringPiSPI_{}'.format(self.__channel), 'w') as f:
            fcntl.flock(f, fcntl.LOCK_EX)
            f.write("{} {} {}".format(6, int(temp//256), int(temp % 256)))
            fcntl.flock(f, fcntl.LOCK_UN)
        

if __name__=='__main__':
    app=QApplication(sys.argv)
    
    # Ermittle die Kanäle anhand der Programmparameter
    channels = { int(x) for x in sys.argv[1:]}
    if not channels:
        channels={0}

    # Erstelle für jeden angegebenen Kanal ein Fenster
    windows=[]
    for ch in channels:
        win=MyMainWindow(ch)
        win.show()
        windows.append(win)
    
    res=app.exec()
    # Bereinige die Sensorsimulation
    for ch in channels:
        try:
            os.remove('/tmp/wiringPiSPI_{}'.format(ch))
        except:
            print('Der Temperatursensor (Channel {}) konnte nicht entfernt werden.'.
                   format(ch))
            
    sys.exit(res)
    
