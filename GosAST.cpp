#include "GosAST.h"

namespace Gos {
    static inline std::string gosASTFileName;
    static inline int gosASTOffError = 0;
    static inline bool gosASTHasError = false;
    void GosASTError::SetFileName(const std::string& name) {
        gosASTFileName = name;
    }
    void GosASTError::UnexpectedToken(GosToken token) {
        gosASTHasError = true;
        if (!gosASTOffError) {
            std::cerr << "Error: " << gosASTFileName << ": " << token.line << ": unexpected token: \"" << token.ToString() << "\"" << std::endl;
        }/* else {
            std::cerr << "offerror: ";
            std::cerr << "Error: " << gosASTFileName << ": " << token.line << ": unexpected token: \"" << token.ToString() << "\"" << std::endl;
        }*/
    }
    void GosASTError::ErrorToken(GosToken token, GosToken correct) {
        gosASTHasError = true;
        if (!gosASTOffError) {
            std::cerr << "Error: " << gosASTFileName << ": " << token.line << ": unexpected token \"" << token.ToString() << "\" instead of \"" << correct.ToString() << "\"" << std::endl;
        }/* else {
            std::cerr << "offerror: ";
            std::cerr << "Error: " << gosASTFileName << ": " << token.line << ": unexpected token \"" << token.ToString() << "\" instead of \"" << correct.ToString() << "\"" << std::endl;
        }*/
    }
}

#undef Build
#undef Print

int Gos::GosAST::CompileAST(Compiler::VMCompiler& code) {
    code.SetProgramFileName(gosASTFileName);
    auto setMemsize = code.vm.WriteAlloc();
    for (auto* ast : nodes) {
        ast->CompileAST(code);
    }
    for (int i = 0; i < code.lambdaDefStartPos.size(); i++) {
        code.vm.WriteCommandJmp()(code.lambdaDefStartPos[i]);
        code.lambdaDefJumpout[i](code.vm.GetProgress());
    }
    setMemsize(code.pos);
    code.vm.WriteFinish();
    return 0;
}

void Gos::GosAST::Build(Gos::GosTokenizer& tokenizer) {
    gosASTOffError = 0;
    gosASTHasError = false;
    while (!tokenizer.IsEOF() && !gosASTHasError) {
        nodes.push_back(new AST::Preprocess(tokenizer));
    }
}

void Gos::GosAST::PrintAST(int indent) {
    for (int i = 0; i < indent; i++) {
        std::cout << "  ";
    }
    for (auto* ast : nodes) {
        ast->PrintAST(indent);
    }
}

Gos::GosAST::~GosAST() {
    for (auto* ast : nodes) {
        delete ast;
    }
}

#define Print(name) void name::PrintAST(int indent)
#define Build(name) name::name(Gos::GosTokenizer& tokenizer)
#define Expect(name) nodes.push_back(new name(tokenizer))
#define Log(name) branch = 0; if (gosASTHasError) return; /*std::cout << "Building " << #name << std::endl;*/

namespace Gos { namespace AST {
    // Build Functions
    Empty::Empty() {}
    Build(Symbol) {
        Log(Symbol);
        token = tokenizer.GetToken();
    }
    Build(Number) {
        Log(Number);
        token = tokenizer.GetToken();
    }
    Build(String) {
        Log(String);
        token = tokenizer.GetToken();
    }
    Build(Primary) {
        Log(Primary);
        token = tokenizer.GetToken();
        branch = 0;
        if (token.type == L_ROUND) {
            Expect(Exp);
            tokenizer.EatToken(R_ROUND);
        } else if (token.type == LAMBDA) {
            tokenizer.BackToken();
            Expect(LambdaDef);
        } else if (token.type == NEW) {
            branch = 1;
            Expect(Symbol);
            if (tokenizer.GetToken().type == L_ROUND) {
                if (tokenizer.GetToken().type != R_ROUND) {
                    branch = 3;
                    tokenizer.BackToken();
                    Expect(ArgsList);
                    tokenizer.EatToken(R_ROUND);
                }
            } else {
                tokenizer.BackToken();
            }
        } else {
            tokenizer.BackToken();
            if (token.type == SYMBOL) {
                Expect(Symbol);
            } else if (token.type == NUMBER) {
                Expect(Number);
            } else if (token.type == STRING) {
                Expect(String);
            } else {
                GosASTError::UnexpectedToken(token);
            }
        }
    }
    Build(PostFix) {
        Log(PostFix);
        Expect(Primary);
        while (true) {
            GosAST* rt = new Empty();
            nodes.push_back(rt);
            rt->token = tokenizer.GetToken();
            if (rt->token.type == L_SQUARE) {
                rt->branch = 1;
                rt->Expect(Exp);
                tokenizer.EatToken(R_SQUARE);
            } else if (rt->token.type == L_ROUND) {
                rt->branch = 2;
                if (tokenizer.GetToken().type != R_ROUND) {
                    tokenizer.BackToken();
                    rt->Expect(ArgsList);
                    tokenizer.EatToken(R_ROUND);
                }
            } else if (rt->token.type == DOT) {
                rt->branch = 3;
                rt->Expect(Symbol);
                if (tokenizer.GetToken().type == L_ROUND) {
                    rt->branch = 4;
                    if (tokenizer.GetToken().type != R_ROUND) {
                        tokenizer.BackToken();
                        rt->Expect(ArgsList);
                        tokenizer.EatToken(R_ROUND);
                    }
                } else {
                    tokenizer.BackToken();
                }
            } else if (rt->token.type == INC || rt->token.type == DEC) {
                rt->branch = 5;
                if (rt->token.type == INC) {
                    rt->branch = 6;
                }
                // DO NOTHING
            } else {
                token.type = NONE;
                tokenizer.BackToken();
                delete nodes[nodes.size() - 1];
                nodes.pop_back();
                break;
            }
        }
    }
    Build(Unary) {
        Log(Unary);
        token = tokenizer.GetToken();
        if (token.type == ADD || token.type == SUB || token.type == NOT || token.type == MUL || token.type == ADDR) {
            branch = 1;
            Expect(Unary);
        } else {
            tokenizer.BackToken();
            Expect(PostFix);
        }
    }
    Build(Cast) {
        Log(Cast);
        Expect(Unary);
        token = tokenizer.GetToken();
        if (token.type == AS) {
            Expect(Symbol);
        } else {
            tokenizer.BackToken();
        }
    }
    Build(Mul) {
        Log(Mul);
        Expect(Cast);
        token = tokenizer.GetToken();
        while (token.type == MUL || token.type == DIV || token.type == REM) {
            Expect(Cast);
            tokenList.push_back(token);
            token = tokenizer.GetToken();
        }
        tokenizer.BackToken();
    }
    Build(Add) {
        Log(Add);
        Expect(Mul);
        token = tokenizer.GetToken();
        while (token.type == ADD || token.type == SUB) {
            Expect(Mul);
            tokenList.push_back(token);
            token = tokenizer.GetToken();
        }
        tokenizer.BackToken();
    }
    Build(Relation) {
        Log(Relation);
        Expect(Add);
        token = tokenizer.GetToken();
        while (token.type == L || token.type == LE || token.type == G || token.type == GE) {
            Expect(Add);
            tokenList.push_back(token);
            token = tokenizer.GetToken();
        }
        tokenizer.BackToken();
    }
    Build(Equality) {
        Log(Equality);
        Expect(Relation);
        token = tokenizer.GetToken();
        while (token.type == E || token.type == NE) {
            Expect(Relation);
            tokenList.push_back(token);
            token = tokenizer.GetToken();
        }
        tokenizer.BackToken();
    }
    Build(LogicAnd) {
        Log(LogicAnd);
        Expect(Equality);
        token = tokenizer.GetToken();
        while (token.type == AND) {
            Expect(Equality);
            tokenList.push_back(token);
            token = tokenizer.GetToken();
        }
        tokenizer.BackToken();
    }
    Build(LogicOr) {
        Log(LogicOr);
        Expect(LogicAnd);
        token = tokenizer.GetToken();
        while (token.type == OR) {
            Expect(LogicAnd);
            tokenList.push_back(token);
            token = tokenizer.GetToken();
        }
        tokenizer.BackToken();
    }
    Build(Cond) {
        Log(Cond);
        Expect(LogicOr);
        token = tokenizer.GetToken();
        if (token.type == QUESTION) {
            branch = 1;
            Expect(Exp);
            tokenizer.EatToken(COLON);
            Expect(Exp);
        } else {
            tokenizer.BackToken();
        }
    }
    Build(Const) {
        Log(Const);
        Expect(Cond);
    }
    Build(Assign) {
        Log(Assign);
        Expect(Unary);
        token = tokenizer.GetToken();
        if (token.type != ASSIGN && token.type != ASSIGN_ADD &&
            token.type != ASSIGN_SUB && token.type != ASSIGN_MUL &&
            token.type != ASSIGN_DIV && token.type != ASSIGN_REM &&
            token.type != ASSIGN_XOR && token.type != ASSIGN_AND &&
            token.type != ASSIGN_OR) {
            GosASTError::UnexpectedToken(token);
            return;
        }
        Expect(Cond);
    }
    Build(Exp) {
        Log(Exp);

        gosASTOffError++;
        gosASTHasError = false;
        int progress = tokenizer.GetProgress();
        tokenizer.SetErrorCallback([]() { gosASTHasError = true; return false; });

        Expect(Assign);

        tokenizer.SetErrorCallback([]() { return true; });
        gosASTOffError--;
        if (gosASTHasError) {
            delete nodes[0];
            nodes.pop_back();
        } else {
            return;
        }
        gosASTHasError = false;
        tokenizer.RestoreProgress(progress);

        Expect(Cond);
    }
    Build(DefVar) {
        Log(DefVar);
        Expect(Symbol);
        token = tokenizer.GetToken();
        if (token.type == COLON) {
            branch += 1;
            tokenList.push_back(token);
            Expect(Symbol);
            token = tokenizer.GetToken();
        }
        if (token.type == ASSIGN) {
            branch += 2;
            tokenList.push_back(token);
            Expect(Exp);
        } else {
            tokenizer.BackToken();
        }
    }
    Build(Statement) {
        Log(Statement);
        token = tokenizer.GetToken();
        if (token.type == VAR) {
            branch = 1;
            Expect(DefVar);
            tokenizer.EatToken(SEM);
        } else if (token.type == IF) {
            branch = 2;
            tokenizer.EatToken(L_ROUND);
            Expect(Exp);
            tokenizer.EatToken(R_ROUND);
            Expect(StatBlock);
            GosToken el = tokenizer.GetToken();
            if (el.type == ELSE) {
                branch = 3;
                el = tokenizer.GetToken();
                if (el.type == IF) {
                    tokenizer.BackToken();
                    Expect(Statement);
                } else {
                    tokenizer.BackToken();
                    Expect(StatBlock);
                }
            } else {
                tokenizer.BackToken();
            }
        } else if (token.type == WHILE) {
            branch = 4;
            tokenizer.EatToken(L_ROUND);
            Expect(Exp);
            tokenizer.EatToken(R_ROUND);
            Expect(StatBlock);
        } else if (token.type == FOR) {
            branch = 5;
            tokenizer.EatToken(L_ROUND);
            GosToken v = tokenizer.GetToken();
            if (v.type == VAR) {
                Expect(DefVar);
            } else {
                tokenizer.BackToken();
                Expect(Exp);
            }
            tokenizer.EatToken(SEM);
            Expect(Exp);
            tokenizer.EatToken(SEM);
            Expect(Exp);
            tokenizer.EatToken(R_ROUND);
            Expect(StatBlock);
        } else if (token.type == FOREACH) {
            branch = 6;
            tokenizer.EatToken(L_ROUND);
            tokenizer.EatToken(VAR);
            Expect(Symbol);
            tokenizer.EatToken(COLON);
            Expect(Exp);
            tokenizer.EatToken(R_ROUND);
            Expect(StatBlock);
        } else if (token.type == RETURN) {
            branch = 7;
            if (tokenizer.GetToken().type != SEM) {
                tokenizer.BackToken();
                Expect(Exp);
                tokenizer.EatToken(SEM);
            } else {
                nodes.push_back(new Empty());
                nodes[0]->branch = -1;
            }
        } else if (token.type == BREAK) {
            branch = 8;
            tokenizer.EatToken(SEM);
        } else if (token.type == CONTINUE) {
            branch = 9;
            tokenizer.EatToken(SEM);
        } else {
            branch = 10;
            tokenizer.BackToken();
            Expect(Exp);
            tokenizer.EatToken(SEM);
        }
    }
    Build(StatBlock) {
        Log(StatBlock);
        tokenizer.EatToken(L_CURLY);
        token = tokenizer.GetToken();
        while (token.type != R_CURLY) {
            tokenizer.BackToken();
            Expect(Statement);
            if (gosASTHasError) {
                return;
            }
            token = tokenizer.GetToken();
        }
    }
    Build(DefArg) {
        Log(DefArg);
        Expect(Symbol);
        tokenizer.EatToken(COLON);
        Expect(Symbol);
    }
    Build(DefArgList) {
        Log(DefArgList);
        Expect(DefArg);
        while (!tokenizer.IsEOF() && (token = tokenizer.GetToken()).type == COM) {
            Expect(DefArg);
        }
        tokenizer.BackToken();
    }
    Build(ArgsList) {
        Log(ArgsList);
        Expect(Exp);
        while (!tokenizer.IsEOF() && (token = tokenizer.GetToken()).type == COM) {
            Expect(Exp);
        }
        tokenizer.BackToken();
    }
    Build(LambdaDef) {
        Log(LambdaDef);
        tokenizer.EatToken(LAMBDA);
        if (tokenizer.GetToken().type == L_SQUARE) {
            if (tokenizer.GetToken().type != R_SQUARE) {
                tokenizer.BackToken();
                branch += 1;
                nodes.push_back(new Empty());
                if (tokenizer.GetToken().type == ADDR) {
                    nodes[0]->Expect(Symbol);
                    nodes[0]->nodes.back()->branch = 1;
                } else {
                    tokenizer.BackToken();
                    nodes[0]->Expect(Symbol);
                    nodes[0]->nodes.back()->branch = 0;
                }
                while (!tokenizer.IsEOF() && (token = tokenizer.GetToken()).type == COM) {
                    if (tokenizer.GetToken().type == ADDR) {
                        nodes[0]->Expect(Symbol);
                        nodes[0]->nodes.back()->branch = 1;
                    } else {
                        tokenizer.BackToken();
                        nodes[0]->Expect(Symbol);
                        nodes[0]->nodes.back()->branch = 0;
                    }
                }
                tokenizer.BackToken();
                tokenizer.EatToken(R_SQUARE);
            }
        } else {
            tokenizer.BackToken();
        }
        tokenizer.EatToken(L_ROUND);
        token = tokenizer.GetToken();
        tokenizer.BackToken();
        if (token.type != R_ROUND) {
            branch += 2;
            Expect(DefArgList);
        }
        tokenizer.EatToken(R_ROUND);
        Expect(Symbol);
        Expect(StatBlock);
    }
    Build(ClassDef) {
        Log(ClassDef);
        token = tokenizer.GetToken();
        tokenizer.BackToken();
        if (token.type == L_SQUARE) {
            branch += 1;
            Expect(Attribute);
        }
        token = tokenizer.GetToken();
        if (token.type == FUNC) {
            branch += 2;
            Expect(Symbol);
            tokenizer.EatToken(L_ROUND);
            token = tokenizer.GetToken();
            tokenizer.BackToken();
            if (token.type != R_ROUND) {
                branch += 8;
                Expect(DefArgList);
            }
            tokenizer.EatToken(R_ROUND);
            Expect(Symbol);
            Expect(StatBlock);
        } else if (token.type == VAR) {
            branch += 4;
            Expect(Symbol);
            tokenizer.EatToken(COLON);
            Expect(Symbol);
            tokenizer.EatToken(SEM);
        } else if (token.type == USING) {
            branch += 16;
            Expect(Symbol);
            tokenizer.EatToken(SEM);
        }
    }
    Build(IDList) {
        Log(IDList);
        Expect(Symbol);
        while (!tokenizer.IsEOF() && (token = tokenizer.GetToken()).type == COM) {
            Expect(Symbol);
        }
        tokenizer.BackToken();
    }
    Build(SingleAttr) {
        Log(SingleAttr);
        Expect(Symbol);
        token = tokenizer.GetToken();
        if (token.type == L_ROUND) {
            token = tokenizer.GetToken();
            if (token.type != R_ROUND) {
                tokenizer.BackToken();
                Expect(IDList);
                tokenizer.EatToken(R_ROUND);
            }
        } else {
            tokenizer.BackToken();
        }
    }
    Build(Attribute) {
        Log(Attribute);
        tokenizer.EatToken(L_SQUARE);
        Expect(SingleAttr);
        while (!tokenizer.IsEOF() && (token = tokenizer.GetToken()).type != R_SQUARE) {
            if (token.type != COM) {
                GosASTError::ErrorToken(token, { COM, token.line });
                return;
            }
            Expect(SingleAttr);
        }
    }
    Build(Preprocess) {
        Log(Preprocess);
        token = tokenizer.GetToken();
        if (token.type == USING) {
            branch = 8;
            Expect(Symbol);
            tokenizer.EatToken(SEM);
            return;
        }
        branch = 1;
        tokenizer.BackToken();
        if (token.type == L_SQUARE) {
            branch += 2;
            Expect(Attribute);
        }
        tokenizer.EatToken(CLASS);
        Expect(Symbol);
        token = tokenizer.GetToken();
        if (token.type == COLON) {
            branch += 4;
            Expect(IDList);
        } else {
            tokenizer.BackToken();
        }
        tokenizer.EatToken(L_CURLY);
        while (!tokenizer.IsEOF() && (token = tokenizer.GetToken()).type != R_CURLY) {
            if (gosASTHasError) {
                return;
            }
            tokenizer.BackToken();
            Expect(ClassDef);
        }
    }

    // Print Functions

#define PrintIndent() for (int i = 0; i < indent; i++) std::cout << "  ";
#define SkipIf(cond) if (nodes.size() > 0 && (cond)) { nodes[0]->PrintAST(indent); return; }
#define PrintName() std::string name = (std::string)TypeID::get<decltype(*this)>().getName(); name = name.substr(10, name.length() - 11); std::cout << name;
#define PrintNodes() std::cout << std::endl; for (auto* ast : nodes) ast->PrintAST(indent + 1);
#define PrintToken() std::cout << " " << token.ToString();
#define PrintTokenList() for (auto t : tokenList) std::cout << " " << t.ToString();

    Print(Empty) {
        PrintIndent();
        std::cout << "-";
        PrintToken();
        PrintNodes();
    }
    Print(Symbol) {
        PrintIndent();
        PrintName();
        PrintToken();
        PrintNodes();
    }
    Print(Number) {
        PrintIndent();
        PrintName();
        PrintToken();
        PrintNodes();
    }
    Print(String) {
        PrintIndent();
        PrintName();
        PrintToken();
        PrintNodes();
    }
    Print(Primary) {
        SkipIf(token.type != L_SQUARE);
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(PostFix) {
        SkipIf(nodes.size() == 1);
        PrintIndent();
        PrintName();
        PrintToken();
        PrintNodes();
    }
    Print(Unary) {
        SkipIf(token.type != ADD && token.type != SUB && token.type != NOT);
        PrintIndent();
        PrintName();
        PrintToken();
        PrintNodes();
    }
    Print(Cast) {
        SkipIf(token.type != AS);
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(Mul) {
        SkipIf(nodes.size() == 1);
        PrintIndent();
        PrintName();
        PrintTokenList();
        PrintNodes();
    }
    Print(Add) {
        SkipIf(nodes.size() == 1);
        PrintIndent();
        PrintName();
        PrintTokenList();
        PrintNodes();
    }
    Print(Relation) {
        SkipIf(nodes.size() == 1);
        PrintIndent();
        PrintName();
        PrintTokenList();
        PrintNodes();
    }
    Print(Equality) {
        SkipIf(nodes.size() == 1);
        PrintIndent();
        PrintName();
        PrintTokenList();
        PrintNodes();
    }
    Print(LogicAnd) {
        SkipIf(nodes.size() == 1);
        PrintIndent();
        PrintName();
        PrintTokenList();
        PrintNodes();
    }
    Print(LogicOr) {
        SkipIf(nodes.size() == 1);
        PrintIndent();
        PrintName();
        PrintTokenList();
        PrintNodes();
    }
    Print(Cond) {
        SkipIf(token.type != QUESTION);
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(Const) {
        SkipIf(true);
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(Assign) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(Exp) {
        SkipIf(true);
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(DefVar) {
        PrintIndent();
        PrintName();
        PrintTokenList();
        PrintNodes();
    }
    Print(Statement) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(StatBlock) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(DefArg) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(DefArgList) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(ArgsList) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(LambdaDef) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(ClassDef) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(IDList) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(SingleAttr) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(Attribute) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
    Print(Preprocess) {
        PrintIndent();
        PrintName();
        PrintNodes();
    }
}}
