#include <stddef.h>

#include "stacktraverse.h"

void *
getreturnaddr(int level)
{
    switch(level) {
    case 0: {
        void *fp = __builtin_frame_address(1);
        if (!fp) return NULL;
        return __builtin_return_address(1);
    }
    case 1: {
        void *fp = __builtin_frame_address(2);
        if (!fp) return NULL;
        return __builtin_return_address(2);
    }
    case 2: {
        void *fp = __builtin_frame_address(3);
        if (!fp) return NULL;
        return __builtin_return_address(3);
    }
    case 3: {
        void *fp = __builtin_frame_address(4);
        if (!fp) return NULL;
        return __builtin_return_address(4);
    }
    case 4: {
        void *fp = __builtin_frame_address(5);
        if (!fp) return NULL;
        return __builtin_return_address(5);
    }
    case 5: {
        void *fp = __builtin_frame_address(6);
        if (!fp) return NULL;
        return __builtin_return_address(6);
    }
    case 6: {
        void *fp = __builtin_frame_address(7);
        if (!fp) return NULL;
        return __builtin_return_address(7);
    }
    case 7: {
        void *fp = __builtin_frame_address(8);
        if (!fp) return NULL;
        return __builtin_return_address(8);
    }
    case 8: {
        void *fp = __builtin_frame_address(9);
        if (!fp) return NULL;
        return __builtin_return_address(9);
    }
    case 9: {
        void *fp = __builtin_frame_address(10);
        if (!fp) return NULL;
        return __builtin_return_address(10);
    }
    case 10: {
        void *fp = __builtin_frame_address(11);
        if (!fp) return NULL;
        return __builtin_return_address(11);
    }
    case 11: {
        void *fp = __builtin_frame_address(12);
        if (!fp) return NULL;
        return __builtin_return_address(12);
    }
    case 12: {
        void *fp = __builtin_frame_address(13);
        if (!fp) return NULL;
        return __builtin_return_address(13);
    }
    case 13: {
        void *fp = __builtin_frame_address(14);
        if (!fp) return NULL;
        return __builtin_return_address(14);
    }
    case 14: {
        void *fp = __builtin_frame_address(15);
        if (!fp) return NULL;
        return __builtin_return_address(15);
    }
    case 15: {
        void *fp = __builtin_frame_address(16);
        if (!fp) return NULL;
        return __builtin_return_address(16);
    }
    case 16: {
        void *fp = __builtin_frame_address(17);
        if (!fp) return NULL;
        return __builtin_return_address(17);
    }
    case 17: {
        void *fp = __builtin_frame_address(18);
        if (!fp) return NULL;
        return __builtin_return_address(18);
    }
    case 18: {
        void *fp = __builtin_frame_address(19);
        if (!fp) return NULL;
        return __builtin_return_address(19);
    }
    case 19: {
        void *fp = __builtin_frame_address(20);
        if (!fp) return NULL;
        return __builtin_return_address(20);
    }
    case 20: {
        void *fp = __builtin_frame_address(21);
        if (!fp) return NULL;
        return __builtin_return_address(21);
    }
    case 21: {
        void *fp = __builtin_frame_address(22);
        if (!fp) return NULL;
        return __builtin_return_address(22);
    }
    case 22: {
        void *fp = __builtin_frame_address(23);
        if (!fp) return NULL;
        return __builtin_return_address(23);
    }
    case 23: {
        void *fp = __builtin_frame_address(24);
        if (!fp) return NULL;
        return __builtin_return_address(24);
    }
    case 24: {
        void *fp = __builtin_frame_address(25);
        if (!fp) return NULL;
        return __builtin_return_address(25);
    }
    case 25: {
        void *fp = __builtin_frame_address(26);
        if (!fp) return NULL;
        return __builtin_return_address(26);
    }
    case 26: {
        void *fp = __builtin_frame_address(27);
        if (!fp) return NULL;
        return __builtin_return_address(27);
    }
    case 27: {
        void *fp = __builtin_frame_address(28);
        if (!fp) return NULL;
        return __builtin_return_address(28);
    }
    case 28: {
        void *fp = __builtin_frame_address(29);
        if (!fp) return NULL;
        return __builtin_return_address(29);
    }
    case 29: {
        void *fp = __builtin_frame_address(30);
        if (!fp) return NULL;
        return __builtin_return_address(30);
    }
    case 30: {
        void *fp = __builtin_frame_address(31);
        if (!fp) return NULL;
        return __builtin_return_address(31);
    }
    case 31: {
        void *fp = __builtin_frame_address(32);
        if (!fp) return NULL;
        return __builtin_return_address(32);
    }
    case 32: {
        void *fp = __builtin_frame_address(33);
        if (!fp) return NULL;
        return __builtin_return_address(33);
    }
    case 33: {
        void *fp = __builtin_frame_address(34);
        if (!fp) return NULL;
        return __builtin_return_address(34);
    }
    case 34: {
        void *fp = __builtin_frame_address(35);
        if (!fp) return NULL;
        return __builtin_return_address(35);
    }
    case 35: {
        void *fp = __builtin_frame_address(36);
        if (!fp) return NULL;
        return __builtin_return_address(36);
    }
    case 36: {
        void *fp = __builtin_frame_address(37);
        if (!fp) return NULL;
        return __builtin_return_address(37);
    }
    case 37: {
        void *fp = __builtin_frame_address(38);
        if (!fp) return NULL;
        return __builtin_return_address(38);
    }
    case 38: {
        void *fp = __builtin_frame_address(39);
        if (!fp) return NULL;
        return __builtin_return_address(39);
    }
    case 39: {
        void *fp = __builtin_frame_address(40);
        if (!fp) return NULL;
        return __builtin_return_address(40);
    }
    case 40: {
        void *fp = __builtin_frame_address(41);
        if (!fp) return NULL;
        return __builtin_return_address(41);
    }
    case 41: {
        void *fp = __builtin_frame_address(42);
        if (!fp) return NULL;
        return __builtin_return_address(42);
    }
    case 42: {
        void *fp = __builtin_frame_address(43);
        if (!fp) return NULL;
        return __builtin_return_address(43);
    }
    case 43: {
        void *fp = __builtin_frame_address(44);
        if (!fp) return NULL;
        return __builtin_return_address(44);
    }
    case 44: {
        void *fp = __builtin_frame_address(45);
        if (!fp) return NULL;
        return __builtin_return_address(45);
    }
    case 45: {
        void *fp = __builtin_frame_address(46);
        if (!fp) return NULL;
        return __builtin_return_address(46);
    }
    case 46: {
        void *fp = __builtin_frame_address(47);
        if (!fp) return NULL;
        return __builtin_return_address(47);
    }
    case 47: {
        void *fp = __builtin_frame_address(48);
        if (!fp) return NULL;
        return __builtin_return_address(48);
    }
    case 48: {
        void *fp = __builtin_frame_address(49);
        if (!fp) return NULL;
        return __builtin_return_address(49);
    }
    case 49: {
        void *fp = __builtin_frame_address(50);
        if (!fp) return NULL;
        return __builtin_return_address(50);
    }
    case 50: {
        void *fp = __builtin_frame_address(51);
        if (!fp) return NULL;
        return __builtin_return_address(51);
    }
    case 51: {
        void *fp = __builtin_frame_address(52);
        if (!fp) return NULL;
        return __builtin_return_address(52);
    }
    case 52: {
        void *fp = __builtin_frame_address(53);
        if (!fp) return NULL;
        return __builtin_return_address(53);
    }
    case 53: {
        void *fp = __builtin_frame_address(54);
        if (!fp) return NULL;
        return __builtin_return_address(54);
    }
    case 54: {
        void *fp = __builtin_frame_address(55);
        if (!fp) return NULL;
        return __builtin_return_address(55);
    }
    case 55: {
        void *fp = __builtin_frame_address(56);
        if (!fp) return NULL;
        return __builtin_return_address(56);
    }
    case 56: {
        void *fp = __builtin_frame_address(57);
        if (!fp) return NULL;
        return __builtin_return_address(57);
    }
    case 57: {
        void *fp = __builtin_frame_address(58);
        if (!fp) return NULL;
        return __builtin_return_address(58);
    }
    case 58: {
        void *fp = __builtin_frame_address(59);
        if (!fp) return NULL;
        return __builtin_return_address(59);
    }
    case 59: {
        void *fp = __builtin_frame_address(60);
        if (!fp) return NULL;
        return __builtin_return_address(60);
    }
    case 60: {
        void *fp = __builtin_frame_address(61);
        if (!fp) return NULL;
        return __builtin_return_address(61);
    }
    case 61: {
        void *fp = __builtin_frame_address(62);
        if (!fp) return NULL;
        return __builtin_return_address(62);
    }
    case 62: {
        void *fp = __builtin_frame_address(63);
        if (!fp) return NULL;
        return __builtin_return_address(63);
    }
    case 63: {
        void *fp = __builtin_frame_address(64);
        if (!fp) return NULL;
        return __builtin_return_address(64);
    }
    case 64: {
        void *fp = __builtin_frame_address(65);
        if (!fp) return NULL;
        return __builtin_return_address(65);
    }
    case 65: {
        void *fp = __builtin_frame_address(66);
        if (!fp) return NULL;
        return __builtin_return_address(66);
    }
    case 66: {
        void *fp = __builtin_frame_address(67);
        if (!fp) return NULL;
        return __builtin_return_address(67);
    }
    case 67: {
        void *fp = __builtin_frame_address(68);
        if (!fp) return NULL;
        return __builtin_return_address(68);
    }
    case 68: {
        void *fp = __builtin_frame_address(69);
        if (!fp) return NULL;
        return __builtin_return_address(69);
    }
    case 69: {
        void *fp = __builtin_frame_address(70);
        if (!fp) return NULL;
        return __builtin_return_address(70);
    }
    case 70: {
        void *fp = __builtin_frame_address(71);
        if (!fp) return NULL;
        return __builtin_return_address(71);
    }
    case 71: {
        void *fp = __builtin_frame_address(72);
        if (!fp) return NULL;
        return __builtin_return_address(72);
    }
    case 72: {
        void *fp = __builtin_frame_address(73);
        if (!fp) return NULL;
        return __builtin_return_address(73);
    }
    case 73: {
        void *fp = __builtin_frame_address(74);
        if (!fp) return NULL;
        return __builtin_return_address(74);
    }
    case 74: {
        void *fp = __builtin_frame_address(75);
        if (!fp) return NULL;
        return __builtin_return_address(75);
    }
    case 75: {
        void *fp = __builtin_frame_address(76);
        if (!fp) return NULL;
        return __builtin_return_address(76);
    }
    case 76: {
        void *fp = __builtin_frame_address(77);
        if (!fp) return NULL;
        return __builtin_return_address(77);
    }
    case 77: {
        void *fp = __builtin_frame_address(78);
        if (!fp) return NULL;
        return __builtin_return_address(78);
    }
    case 78: {
        void *fp = __builtin_frame_address(79);
        if (!fp) return NULL;
        return __builtin_return_address(79);
    }
    case 79: {
        void *fp = __builtin_frame_address(80);
        if (!fp) return NULL;
        return __builtin_return_address(80);
    }
    case 80: {
        void *fp = __builtin_frame_address(81);
        if (!fp) return NULL;
        return __builtin_return_address(81);
    }
    case 81: {
        void *fp = __builtin_frame_address(82);
        if (!fp) return NULL;
        return __builtin_return_address(82);
    }
    case 82: {
        void *fp = __builtin_frame_address(83);
        if (!fp) return NULL;
        return __builtin_return_address(83);
    }
    case 83: {
        void *fp = __builtin_frame_address(84);
        if (!fp) return NULL;
        return __builtin_return_address(84);
    }
    case 84: {
        void *fp = __builtin_frame_address(85);
        if (!fp) return NULL;
        return __builtin_return_address(85);
    }
    case 85: {
        void *fp = __builtin_frame_address(86);
        if (!fp) return NULL;
        return __builtin_return_address(86);
    }
    case 86: {
        void *fp = __builtin_frame_address(87);
        if (!fp) return NULL;
        return __builtin_return_address(87);
    }
    case 87: {
        void *fp = __builtin_frame_address(88);
        if (!fp) return NULL;
        return __builtin_return_address(88);
    }
    case 88: {
        void *fp = __builtin_frame_address(89);
        if (!fp) return NULL;
        return __builtin_return_address(89);
    }
    case 89: {
        void *fp = __builtin_frame_address(90);
        if (!fp) return NULL;
        return __builtin_return_address(90);
    }
    case 90: {
        void *fp = __builtin_frame_address(91);
        if (!fp) return NULL;
        return __builtin_return_address(91);
    }
    case 91: {
        void *fp = __builtin_frame_address(92);
        if (!fp) return NULL;
        return __builtin_return_address(92);
    }
    case 92: {
        void *fp = __builtin_frame_address(93);
        if (!fp) return NULL;
        return __builtin_return_address(93);
    }
    case 93: {
        void *fp = __builtin_frame_address(94);
        if (!fp) return NULL;
        return __builtin_return_address(94);
    }
    case 94: {
        void *fp = __builtin_frame_address(95);
        if (!fp) return NULL;
        return __builtin_return_address(95);
    }
    case 95: {
        void *fp = __builtin_frame_address(96);
        if (!fp) return NULL;
        return __builtin_return_address(96);
    }
    case 96: {
        void *fp = __builtin_frame_address(97);
        if (!fp) return NULL;
        return __builtin_return_address(97);
    }
    case 97: {
        void *fp = __builtin_frame_address(98);
        if (!fp) return NULL;
        return __builtin_return_address(98);
    }
    case 98: {
        void *fp = __builtin_frame_address(99);
        if (!fp) return NULL;
        return __builtin_return_address(99);
    }
    case 99: {
        void *fp = __builtin_frame_address(100);
        if (!fp) return NULL;
        return __builtin_return_address(100);
    }
    case 100: {
        void *fp = __builtin_frame_address(101);
        if (!fp) return NULL;
        return __builtin_return_address(101);
    }
    case 101: {
        void *fp = __builtin_frame_address(102);
        if (!fp) return NULL;
        return __builtin_return_address(102);
    }
    case 102: {
        void *fp = __builtin_frame_address(103);
        if (!fp) return NULL;
        return __builtin_return_address(103);
    }
    case 103: {
        void *fp = __builtin_frame_address(104);
        if (!fp) return NULL;
        return __builtin_return_address(104);
    }
    case 104: {
        void *fp = __builtin_frame_address(105);
        if (!fp) return NULL;
        return __builtin_return_address(105);
    }
    case 105: {
        void *fp = __builtin_frame_address(106);
        if (!fp) return NULL;
        return __builtin_return_address(106);
    }
    case 106: {
        void *fp = __builtin_frame_address(107);
        if (!fp) return NULL;
        return __builtin_return_address(107);
    }
    case 107: {
        void *fp = __builtin_frame_address(108);
        if (!fp) return NULL;
        return __builtin_return_address(108);
    }
    case 108: {
        void *fp = __builtin_frame_address(109);
        if (!fp) return NULL;
        return __builtin_return_address(109);
    }
    case 109: {
        void *fp = __builtin_frame_address(110);
        if (!fp) return NULL;
        return __builtin_return_address(110);
    }
    case 110: {
        void *fp = __builtin_frame_address(111);
        if (!fp) return NULL;
        return __builtin_return_address(111);
    }
    case 111: {
        void *fp = __builtin_frame_address(112);
        if (!fp) return NULL;
        return __builtin_return_address(112);
    }
    case 112: {
        void *fp = __builtin_frame_address(113);
        if (!fp) return NULL;
        return __builtin_return_address(113);
    }
    case 113: {
        void *fp = __builtin_frame_address(114);
        if (!fp) return NULL;
        return __builtin_return_address(114);
    }
    case 114: {
        void *fp = __builtin_frame_address(115);
        if (!fp) return NULL;
        return __builtin_return_address(115);
    }
    case 115: {
        void *fp = __builtin_frame_address(116);
        if (!fp) return NULL;
        return __builtin_return_address(116);
    }
    case 116: {
        void *fp = __builtin_frame_address(117);
        if (!fp) return NULL;
        return __builtin_return_address(117);
    }
    case 117: {
        void *fp = __builtin_frame_address(118);
        if (!fp) return NULL;
        return __builtin_return_address(118);
    }
    case 118: {
        void *fp = __builtin_frame_address(119);
        if (!fp) return NULL;
        return __builtin_return_address(119);
    }
    case 119: {
        void *fp = __builtin_frame_address(120);
        if (!fp) return NULL;
        return __builtin_return_address(120);
    }
    case 120: {
        void *fp = __builtin_frame_address(121);
        if (!fp) return NULL;
        return __builtin_return_address(121);
    }
    case 121: {
        void *fp = __builtin_frame_address(122);
        if (!fp) return NULL;
        return __builtin_return_address(122);
    }
    case 122: {
        void *fp = __builtin_frame_address(123);
        if (!fp) return NULL;
        return __builtin_return_address(123);
    }
    case 123: {
        void *fp = __builtin_frame_address(124);
        if (!fp) return NULL;
        return __builtin_return_address(124);
    }
    case 124: {
        void *fp = __builtin_frame_address(125);
        if (!fp) return NULL;
        return __builtin_return_address(125);
    }
    case 125: {
        void *fp = __builtin_frame_address(126);
        if (!fp) return NULL;
        return __builtin_return_address(126);
    }
    case 126: {
        void *fp = __builtin_frame_address(127);
        if (!fp) return NULL;
        return __builtin_return_address(127);
    }
    case 127: {
        void *fp = __builtin_frame_address(128);
        if (!fp) return NULL;
        return __builtin_return_address(128);
    }
    default: return NULL;
    }
}

#include <stddef.h>

#include "stacktraverse.h"

void *
getframeaddr(int level)
{
    switch(level) {
    case 0: return __builtin_frame_address(1);
    case 1: return __builtin_frame_address(2);
    case 2: return __builtin_frame_address(3);
    case 3: return __builtin_frame_address(4);
    case 4: return __builtin_frame_address(5);
    case 5: return __builtin_frame_address(6);
    case 6: return __builtin_frame_address(7);
    case 7: return __builtin_frame_address(8);
    case 8: return __builtin_frame_address(9);
    case 9: return __builtin_frame_address(10);
    case 10: return __builtin_frame_address(11);
    case 11: return __builtin_frame_address(12);
    case 12: return __builtin_frame_address(13);
    case 13: return __builtin_frame_address(14);
    case 14: return __builtin_frame_address(15);
    case 15: return __builtin_frame_address(16);
    case 16: return __builtin_frame_address(17);
    case 17: return __builtin_frame_address(18);
    case 18: return __builtin_frame_address(19);
    case 19: return __builtin_frame_address(20);
    case 20: return __builtin_frame_address(21);
    case 21: return __builtin_frame_address(22);
    case 22: return __builtin_frame_address(23);
    case 23: return __builtin_frame_address(24);
    case 24: return __builtin_frame_address(25);
    case 25: return __builtin_frame_address(26);
    case 26: return __builtin_frame_address(27);
    case 27: return __builtin_frame_address(28);
    case 28: return __builtin_frame_address(29);
    case 29: return __builtin_frame_address(30);
    case 30: return __builtin_frame_address(31);
    case 31: return __builtin_frame_address(32);
    case 32: return __builtin_frame_address(33);
    case 33: return __builtin_frame_address(34);
    case 34: return __builtin_frame_address(35);
    case 35: return __builtin_frame_address(36);
    case 36: return __builtin_frame_address(37);
    case 37: return __builtin_frame_address(38);
    case 38: return __builtin_frame_address(39);
    case 39: return __builtin_frame_address(40);
    case 40: return __builtin_frame_address(41);
    case 41: return __builtin_frame_address(42);
    case 42: return __builtin_frame_address(43);
    case 43: return __builtin_frame_address(44);
    case 44: return __builtin_frame_address(45);
    case 45: return __builtin_frame_address(46);
    case 46: return __builtin_frame_address(47);
    case 47: return __builtin_frame_address(48);
    case 48: return __builtin_frame_address(49);
    case 49: return __builtin_frame_address(50);
    case 50: return __builtin_frame_address(51);
    case 51: return __builtin_frame_address(52);
    case 52: return __builtin_frame_address(53);
    case 53: return __builtin_frame_address(54);
    case 54: return __builtin_frame_address(55);
    case 55: return __builtin_frame_address(56);
    case 56: return __builtin_frame_address(57);
    case 57: return __builtin_frame_address(58);
    case 58: return __builtin_frame_address(59);
    case 59: return __builtin_frame_address(60);
    case 60: return __builtin_frame_address(61);
    case 61: return __builtin_frame_address(62);
    case 62: return __builtin_frame_address(63);
    case 63: return __builtin_frame_address(64);
    case 64: return __builtin_frame_address(65);
    case 65: return __builtin_frame_address(66);
    case 66: return __builtin_frame_address(67);
    case 67: return __builtin_frame_address(68);
    case 68: return __builtin_frame_address(69);
    case 69: return __builtin_frame_address(70);
    case 70: return __builtin_frame_address(71);
    case 71: return __builtin_frame_address(72);
    case 72: return __builtin_frame_address(73);
    case 73: return __builtin_frame_address(74);
    case 74: return __builtin_frame_address(75);
    case 75: return __builtin_frame_address(76);
    case 76: return __builtin_frame_address(77);
    case 77: return __builtin_frame_address(78);
    case 78: return __builtin_frame_address(79);
    case 79: return __builtin_frame_address(80);
    case 80: return __builtin_frame_address(81);
    case 81: return __builtin_frame_address(82);
    case 82: return __builtin_frame_address(83);
    case 83: return __builtin_frame_address(84);
    case 84: return __builtin_frame_address(85);
    case 85: return __builtin_frame_address(86);
    case 86: return __builtin_frame_address(87);
    case 87: return __builtin_frame_address(88);
    case 88: return __builtin_frame_address(89);
    case 89: return __builtin_frame_address(90);
    case 90: return __builtin_frame_address(91);
    case 91: return __builtin_frame_address(92);
    case 92: return __builtin_frame_address(93);
    case 93: return __builtin_frame_address(94);
    case 94: return __builtin_frame_address(95);
    case 95: return __builtin_frame_address(96);
    case 96: return __builtin_frame_address(97);
    case 97: return __builtin_frame_address(98);
    case 98: return __builtin_frame_address(99);
    case 99: return __builtin_frame_address(100);
    case 100: return __builtin_frame_address(101);
    case 101: return __builtin_frame_address(102);
    case 102: return __builtin_frame_address(103);
    case 103: return __builtin_frame_address(104);
    case 104: return __builtin_frame_address(105);
    case 105: return __builtin_frame_address(106);
    case 106: return __builtin_frame_address(107);
    case 107: return __builtin_frame_address(108);
    case 108: return __builtin_frame_address(109);
    case 109: return __builtin_frame_address(110);
    case 110: return __builtin_frame_address(111);
    case 111: return __builtin_frame_address(112);
    case 112: return __builtin_frame_address(113);
    case 113: return __builtin_frame_address(114);
    case 114: return __builtin_frame_address(115);
    case 115: return __builtin_frame_address(116);
    case 116: return __builtin_frame_address(117);
    case 117: return __builtin_frame_address(118);
    case 118: return __builtin_frame_address(119);
    case 119: return __builtin_frame_address(120);
    case 120: return __builtin_frame_address(121);
    case 121: return __builtin_frame_address(122);
    case 122: return __builtin_frame_address(123);
    case 123: return __builtin_frame_address(124);
    case 124: return __builtin_frame_address(125);
    case 125: return __builtin_frame_address(126);
    case 126: return __builtin_frame_address(127);
    case 127: return __builtin_frame_address(128);
    default: return NULL;
    }
}
