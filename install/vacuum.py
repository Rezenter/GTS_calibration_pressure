from mcculw import ul
from mcculw.enums import ULRange
from mcculw.ul import ULError
from time import sleep, gmtime, strftime

board_num = 0
channel = 0
ai_range = ULRange.BIP10VOLTS

channels = [0, 1] # 0 = iBaratron, 1 = setra


def voltage2pressure(voltage, ch):
    if ch == 0:
        return
    if ch == 1:
        return voltage * (10 / 10)


while 1:
    try:
        print(strftime("%H:%M:%S", gmtime()))
        for ch_ind in range(len(channels)):
            ch = channels[ch_ind]
            raw = ul.a_in(board_num, ch, ai_range)
            voltage = ul.to_eng_units(board_num, ai_range, raw)
            offscale = ''
            if voltage >= 9.5:
                offscale = ' OFFSCALE!'
            if ch == 0:  # i-Baratron
                print('i-Baratron: %.3f V, %.2f torr. %s' % (voltage, voltage * (100 / 10), offscale))
            if ch == 1:  # setra
                print('setra: %.3f V, %.3f torr. %s' % (voltage, voltage * (10 / 10), offscale))
        print('')
    except ULError as e:
        # Display the error
        print("A UL error occurred. Code: " + str(e.errorcode)
              + " Message: " + e.message)
    sleep(1)