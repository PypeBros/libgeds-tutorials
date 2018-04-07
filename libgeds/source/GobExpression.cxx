#ifndef GOB_EXPRESSION_H
#define GOB_EXPRESSION_H

template<typename T> class GobExpression : iReport {
  NOCOPY(GobExpression);
  static T* tank;
  static const int REGISTERS = 16;
  static const int STACKSIZE = 16;
  u8 *code; s16 *imm;
public:
  INTANK();
  GobExpression<T>() :
    code(0), imm(0) {
  }
  ~GobExpression() {
    if (code) tank->release(code);
    if (imm) tank->release(imm);
  }
private:
  static void setContext(GameObject *g, s16* cd=0) {
  }
  static GameObject* getContext() {
    /** used by InspectorWidget */
    return 0; //context;
  }

  static bool push_imm(std::vector<ActionOpcodes> &code, std::vector<s16> &imm, int val) {
    unsigned at=imm.size();
    if (at>15) { step("expression too long"); return false; }
    imm.push_back((s16)val);
    code.push_back((ActionOpcodes)((int)OP_CONSTANT+at));
    return true;
  }

  static int push_regnum(std::vector<ActionOpcodes> &code, char d, ActionOpcodes op) {
    if (d>='0' && d<='9') d=d-'0';
    else d=10+d-(d>'F'?'a':'A');
    if (d>REGISTERS) {
      step("'%c' is out of 0..15 range",d);
      throw iScriptException("invalid register number");
    }
    code.push_back((ActionOpcodes)((int)op+d));
    return d;
  }


  bool oops(const char* msg) {
    iprintf("code: %2x%2x%2x%2x %s",code[0],code[1],code[2],code[3],msg);
    return false;
  }
public:
  bool eval(s16 data[REGISTERS]) {
    GobCollision gc[2]={{0,0,data},{0,0,0}};
    return eval(gc);
  }

  bool eval(GobCollision* c) {
    s16 *data=c[0].data;
    s16 stack[STACKSIZE]; int sp=0;
    u8 op;
    if (!code) return true;
    u8* pc = code; // the program counter.

    while((op=*pc++)!=OP_DONE) {
      switch(op&0xf0) {
      case OP_GETVAR: // 1x
	if (sp>=STACKSIZE-1) return oops("!VAR");
	stack[sp++]=data[op&0xf];
	break;
      case OP_SETVAR: // 2x
	if (sp==0) return oops("!SET");
	//	printf("v%i := %4x",op&0xf,stack[sp-1]);
	data[op&0xf]=stack[--sp];
	break;
      case OP_CONSTANT: // 3x
	if (sp>=STACKSIZE-1) return oops("!CST");
	stack[sp++]=imm[op&0xf]; // printf("imm(%i,%x)",op&0xf,imm[op&0xf]);
	break;
      case OP_ADD...OP_EQ: {
	if (sp<=1) return oops("!OP!");
	s16 b=stack[--sp];
	s16 a=stack[sp-1];
	// printf("(%2x,%1x: a=%4x, b=%4x)",op,sp,(int)a,(int)b);
	switch(op) {
	case OP_ADD: stack[sp-1]=a+b; break;
	case OP_SUB: stack[sp-1]=a-b; break;
	case OP_MIN: stack[sp-1]=(a<b)?a:b; break;
	case OP_MAX: stack[sp-1]=(a<b)?b:a; break;
	case OP_OR:  stack[sp-1]=a|b; break;
	case OP_AND: stack[sp-1]=a&b; break;
	case OP_XOR: stack[sp-1]=a^b; break;
	case OP_NAND:stack[sp-1]=a&~b; break;
	case OP_MUL: stack[sp-1]=a*b; break;
	case OP_DIV: stack[sp-1]=(b!=0)?a/b:0; break;
	case OP_MOD: stack[sp-1]=(b!=0)?a%b:0; break;
	case OP_EQ:  stack[sp-1]=a==b?1:0; break;
	case OP_NEQ: stack[sp-1]=a==b?0:1; break;
	case OP_LT:  stack[sp-1]=a<b?1:0; break;
	case OP_GT:  stack[sp-1]=a>b?1:0; break;
	case OP_LTE: stack[sp-1]=a<=b?1:0; break;
	case OP_GTE: stack[sp-1]=a>=b?1:0; break;
	case OP_TEST:stack[sp-1]=((a&b)==b)?1:0; break;
	case OP_NTST:stack[sp-1]=((a&b)==b)?0:1; break;
	}
	break;
      }
      case OP_NEG&0xf0:
	switch(op) {
	case OP_NEG:
	  if (sp==0) return oops("!~");
	  stack[sp-1]=-stack[sp-1];
	  break;
	case OP_NOT:
	  if (sp==0) return oops("!!");
	  stack[sp-1]=~stack[sp-1];
	  break;
	case OP_INC:
	  if (sp==0) return oops("!++");
	  stack[sp-1]++;
	  break;
	case OP_DEC:
	  if (sp==0) return oops("!--");
	  stack[sp-1]--;
	  break;
	case OP_TRUE:
	  if (sp>=STACKSIZE-1) return oops("!#T");
	  stack[sp++]=1;
	  break;
	case OP_FALSE:
	  if (sp>=STACKSIZE-1) return oops("!#F");
	  stack[sp++]=0;
	  break;
	case OP_DEBUG:
	  break;
	}
	break;
      case OP_DUP:
	if (sp>=STACKSIZE-1) return oops("!DUP");
	stack[sp++]=stack[op&0xf];
	break;
      default:
	return oops("????");
      }
    }

    return sp>0 && stack[sp-1]!=0;
  }


  bool parse(const char* data) {
        std::vector<ActionOpcodes> tcode;  // temporary constructs
    std::vector<s16> timm;             // will be packed into code and imm.
    while (*data!=0) {
      switch(*data++) {
      case ' ': break;
	/** litteral constants **/
      case '$': { // here comes an hex constant
	int val=0;
	char d;
	while (isxdigit(d=*data)) {
	  if (d>='0' && d<='9') d=d-'0';
	  else d=10+d-(d>'F'?'a':'A');
	  if (d>16) d=0;
	  val=(val<<4)+d;
	  data++;
	}
	if (!push_imm(tcode,timm,val)) return false;
	break;
      }
      case '0'...'9': { // here comes a dec constant
	int val=*(data-1)-'0';
	char d;
	while (isdigit(d=*data)) {
	  d=d-'0';
	  val=val*10+d;
	  data++;
	}
	if (!push_imm(tcode,timm,val)) return false;
	break;
      }
	/** variables manipulation **/
      case 'v': // reads local variable
	  push_regnum(tcode,*data++,OP_GETVAR);
	  break;
      case ':': // changes local variable
	push_regnum(tcode,*data++,OP_SETVAR);
	break;

	/** arithmetic operations **/
      case '+': 
	if (*data=='+') { tcode.push_back(OP_INC); data++; }
	else tcode.push_back(OP_ADD); 
	break;
      case '-': 
	if (*data=='-') { tcode.push_back(OP_DEC); data++; }
	else tcode.push_back(OP_SUB); 
	break;
      case '@':
	push_regnum(tcode,*data++,OP_DUP); 
	break;
      case 'm': tcode.push_back(OP_MIN); break;
      case 'M': tcode.push_back(OP_MAX); break;
      case 't': tcode.push_back(OP_TRUE); break;
      case 'f': tcode.push_back(OP_FALSE); break;
      case '|': tcode.push_back(OP_OR); break;
      case '&': tcode.push_back(OP_AND); break;
      case '^': tcode.push_back(OP_XOR); break;
      case '=': tcode.push_back(OP_EQ); break;
      case '~': tcode.push_back(OP_NEG); break;
      case '/': tcode.push_back(OP_DIV); break;
      case '*': tcode.push_back(OP_MUL); break;
      case '%': tcode.push_back(OP_MOD); break;
      case 'B': tcode.push_back(OP_DEBUG); break;
	 
	case '<':
	  if (*data=='=') {
	    tcode.push_back(OP_LTE); data++; break;
	  } else tcode.push_back(OP_LT); 
	  break;
	case '>':
	  if (*data=='=') {
	    tcode.push_back(OP_GTE); data++; break;
	  } else tcode.push_back(OP_GT);
	  break;
	case '?': tcode.push_back(OP_TEST); break;
	case '!':
	  if (*data=='&') {
	    tcode.push_back(OP_NAND); break;
	  }
	  if (*data=='=') {
	    tcode.push_back(OP_NEQ); data++; break;
	  } else if (*data=='?') {
	    tcode.push_back(OP_NTST); data++; break;
	  } else tcode.push_back(OP_NOT);
	}
      }
      tcode.push_back(OP_DONE);
      u8 *cd=(u8*)tank->place(tcode.size());
      s16* im=(s16*)tank->place(timm.size()*sizeof(s16));
      if (!cd || !im) return false;
      for (uint i=0, n=tcode.size(); i<n; i++) 
	{ cd[i]=tcode[i]; /* printf("%2x",tcode[i]); */ }
      for (uint i=0, n=timm.size(); i<n; i++) im[i]=timm[i];

      code=cd; imm=im;
      //      else { acode=cd; aimm=im; }
      return true;
  }
};
#endif
