// Programmer's Calculator
//
// Copyright 2014, Risto Tiainen
//
// Programmer's Calculator is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// Programmer's Calculator is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Programmer's Calculator.
// If not, see <http://www.gnu.org/licenses/>.

#include "rpnparser.h"
#include <QDebug>

// Check function table for the parser.
// 1. Check for known functions and operators
// 2. Check for hexadecimal and binary prefixes
// 3. Check for decimal point
// 4. Check for valid integer
RPNParser::checkFunction RPNParser::f[] = {
    clr,                // Clear the stack
    swap,               // Swap the two topmost items of the stack
    inv,                // Inverse
    conv,               // Convert (hex2dex, dec2hex)
    asmd,               // Add, multiply, subtract, divide
    hex,                // Input hex
    bin,                // Input binary
    real,               // Input floating point
    integer,            // Input integer
    empty,              // Duplicate head of stack
    (checkFunction)0    // terminator, do not remove
};

RPNParser::RPNParser(CalcStack* _cstack)
{
    cstack = _cstack;
}

bool RPNParser::parse(const QString &in) {
    bool status = false;
    QString s = in;
    checkFunction* fp = &f[0];
    s.replace(" ","");  // Get rid of whitespace

    // Here be parsing algorithm
    // RPN is the easy one and even that is a bit of a bitch
    while(*fp != (checkFunction)0) {
        if((*fp)(this,s)) {
            status=true;
            break;
        }
        fp++;
    }

    return status;
}

bool RPNParser::clr(RPNParser *p, QString &s) {
    bool status = false;

    if(s == "c") {
        p->cstack->clear();
        status=true;
    }

    return status;
}

bool RPNParser::swap(RPNParser *p, QString &s) {
    bool status = false;

    if(s == "s") {
        if(p->cstack->items() > 1) {
            CalcStackItem *a,*b;
            a = p->cstack->popItem();
            b = p->cstack->popItem();
            p->cstack->pushItem(a);
            p->cstack->pushItem(b);
            status=true;
        }
    }

    return status;
}

bool RPNParser::inv(RPNParser *p, QString &s) {
    bool status = false;

    if(s == "i") {
        CalcStackItem *a, *b;

        if(p->cstack->items() > 0) {
            if(p->cstack->top()->isInteger() && p->cstack->top()->getInteger() != 0) {
                a = p->cstack->popItem();
                b = new CalcStackItemInt(1/a->getInteger());
                p->cstack->pushItem(b);
                delete a;
                status = true;
            }
            else if(p->cstack->top()->isFloat() && p->cstack->top()->getFloat() != 0.0) {
                a = p->cstack->popItem();
                b = new CalcStackItemFloat(1.0/a->getFloat());
                p->cstack->pushItem(b);
                delete a;
                status = true;
            }
        }
    }

    return status;
}

bool RPNParser::conv(RPNParser *p, QString &s) {
    bool status=false;

    if(p->cstack->items() > 0 && p->cstack->top()->isInteger()) {
        CalcStackItem* a;

        // 2Dec
        if(s == "d") {
            a = p->cstack->popItem();
            CalcStackItem* b = new CalcStackItemInt(a->getInteger(), 10);
            p->cstack->pushItem(b);
            delete a;
            status=true;
        }
        // 2Hex
        else if(s == "h") {
            a = p->cstack->popItem();
            CalcStackItem* b = new CalcStackItemInt(a->getInteger(), 16);
            p->cstack->pushItem(b);
            delete a;
            status=true;
        }
        // 2Bin
        else if(s == "b") {
            a = p->cstack->popItem();
            CalcStackItem* b = new CalcStackItemInt(a->getInteger(), 2);
            p->cstack->pushItem(b);
            delete a;
            status=true;
        }
    }

    return status;
}

bool RPNParser::asmd(RPNParser* p, QString& s) {
    bool status = false;
    CalcStackItem *a,*b,*c=0;

    if((p->cstack->items()) > 1 && (s == "+" || s == "-" || s == "*" || s == "/")) {
        b = p->cstack->popItem();
        a = p->cstack->popItem();

        if(s == "+") {
            c = CalcStackItem::add(a,b,&status);
        }
        else if(s == "-") {
            c = CalcStackItem::sub(a,b,&status);
        }
        else if(s == "*") {
            c = CalcStackItem::mul(a,b,&status);
        }
        else if(s == "/") {
            c = CalcStackItem::div(a,b,&status);
        }

        if(status) {
            p->cstack->pushItem(c);
            // a and b are gone, free the memory
            delete a;
            delete b;
        }
        else {
            // Could not compute, put the stuff back to the stack
            p->cstack->pushItem(a);
            p->cstack->pushItem(b);
        }
    }
    return status;
}

bool RPNParser::hex(RPNParser *p, QString &s) {
    bool status=false;

    // Let 0x be case sensitive, I don't like it when people write 0X
    if(s.count() > 2 && s.startsWith("0x")) {
        qlonglong i = s.toLongLong(&status, 16);
        if(status) {
            CalcStackItem* newItem = new CalcStackItemInt(i, 16);
            p->cstack->pushItem(newItem);
       }
    }

    return status;
}

bool RPNParser::bin(RPNParser *p, QString &s) {
    bool status=false;

    // Let 0b be case sensitive.
    // toLongLong doesn't recognize the 0b, strip it.
    if(s.count() > 2 && s.startsWith("0b")) {
        qlonglong i = s.remove(0,2).toLongLong(&status, 2);
        if(status) {
            CalcStackItem* newItem = new CalcStackItemInt(i, 2);
            p->cstack->pushItem(newItem);
       }
    }

    return status;
}

bool RPNParser::real(RPNParser *p, QString &s) {
    bool status=false;

    // If there is exactly 1 decimal point in the number,
    // it may be a float.
    if(s.count('.') == 1) {
        qreal f = s.toDouble(&status);
        if(status) {
            CalcStackItem* newItem = new CalcStackItemFloat(f);
            p->cstack->pushItem(newItem);
        }
    }
    return status;
}

bool RPNParser::integer(RPNParser* p, QString& s) {
    bool status = false;
    qlonglong i = s.toLongLong(&status, 10);

    //qDebug() << status;    // TEST

    if(status) {
        CalcStackItem* newItem = new CalcStackItemInt(i);
        p->cstack->pushItem(newItem);
    }

    return status;
}

bool RPNParser::empty(RPNParser *p, QString &s) {
    bool status=false;

    // Duplicate the topmost item in the stack
    if(p->cstack->items() > 0 && s=="") {
        // Once again, I don't know how to do this properly in C++.
        // Will have to switch-case all derived CalcStackItem classes in
        // practice.
        if(p->cstack->top()->isFloat()) {
            CalcStackItemFloat* i = new CalcStackItemFloat((const CalcStackItemFloat&)*p->cstack->top());
            p->cstack->pushItem(i);
        }
        else {
            CalcStackItemInt* i = new CalcStackItemInt((const CalcStackItemInt&)*p->cstack->top());
            p->cstack->pushItem(i);
        }

        status=true;
    }

    return status;
}
