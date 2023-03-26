#include "GosAST.h"

#define Build(name) name(Gos::GosTokenizer& tokenizer)
#define Compile() CompileToVM(GosVM::VMProgram& program)
#define Print() PrintAST(int indent)
#define TokenList std::vector<GosToken> tokenList;

#define GOS_AST(name, extra)                                \
class name : public GosAST {                                \
    public:                                                 \
    Build(name);                                            \
    void Compile() override;                                \
    void Print() override;                                  \
    extra                                                   \
}

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
    namespace AST {
        GOS_AST(Symbol,);
        GOS_AST(Number,);
        GOS_AST(String,);
        GOS_AST(Primary,);
        GOS_AST(PostFix,);
        GOS_AST(Unary,);
        GOS_AST(Cast,);
        GOS_AST(Mul, TokenList);
        GOS_AST(Add, TokenList);
        GOS_AST(Relation, TokenList);
        GOS_AST(Equality, TokenList);
        GOS_AST(LogicAnd, TokenList);
        GOS_AST(LogicOr, TokenList);
        GOS_AST(Cond,);
        GOS_AST(Const,);
        GOS_AST(Assign,);
        GOS_AST(Exp,);
        GOS_AST(DefVar, TokenList);
        GOS_AST(Statement,);
        GOS_AST(StatBlock,);
        GOS_AST(DefArg,);
        GOS_AST(DefArgList,);
        GOS_AST(ArgsList,);
        GOS_AST(LambdaDef,);
        GOS_AST(ClassDef,);
        GOS_AST(IDList,);
        GOS_AST(SingleAttr,);
        GOS_AST(Attribute,);
        GOS_AST(Preprocess,);
    }
}

#undef Compile
#undef Build
#undef Print

void Gos::GosAST::CompileToVM(GosVM::VMProgram& program) {
    for (auto* ast : nodes) {
        ast->CompileToVM(program);
    }
}

void Gos::GosAST::Build(Gos::GosTokenizer& tokenizer) {
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

#define Compile(name) void name::CompileToVM(GosVM::VMProgram& program)
#define Print(name) void name::PrintAST(int indent)
#define Build(name) name::name(Gos::GosTokenizer& tokenizer)
#define Expect(name) nodes.push_back(new name(tokenizer))
#define Log(name) if (gosASTHasError) return; /*std::cout << "Building " << #name << std::endl;*/

namespace Gos { namespace AST {
    // Build Functions
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
        if (token.type == L_ROUND) {
            Expect(Exp);
            tokenizer.EatToken(R_ROUND);
        } else if (token.type == LAMBDA) {
            tokenizer.BackToken();
            Expect(LambdaDef);
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
        token = tokenizer.GetToken();
        if (token.type == L_SQUARE) {
            Expect(Exp);
            tokenizer.EatToken(R_SQUARE);
        } else if (token.type == L_ROUND) {
            if (tokenizer.GetToken().type != R_ROUND) {
                tokenizer.BackToken();
                Expect(ArgsList);
                tokenizer.EatToken(R_ROUND);
            }
        } else if (token.type == DOT) {
            Expect(Symbol);
        } else if (token.type == INC || token.type == DEC) {
            // DO NOTHING
        } else {
            token.type = NONE;
            tokenizer.BackToken();
        }
    }
    Build(Unary) {
        Log(Unary);
        token = tokenizer.GetToken();
        if (token.type == ADD || token.type == SUB || token.type == NOT) {
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
            tokenList.push_back(token);
            Expect(Symbol);
            token = tokenizer.GetToken();
        }
        if (token.type == ASSIGN) {
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
            Expect(DefVar);
            tokenizer.EatToken(SEM);
        } else if (token.type == IF) {
            tokenizer.EatToken(L_ROUND);
            Expect(Exp);
            tokenizer.EatToken(R_ROUND);
            Expect(StatBlock);
            GosToken el = tokenizer.GetToken();
            if (el.type == ELSE) {
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
            tokenizer.EatToken(L_ROUND);
            Expect(Exp);
            tokenizer.EatToken(R_ROUND);
            Expect(StatBlock);
        } else if (token.type == FOR) {
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
            tokenizer.EatToken(L_ROUND);
            tokenizer.EatToken(VAR);
            Expect(Symbol);
            tokenizer.EatToken(COLON);
            Expect(Exp);
            tokenizer.EatToken(R_ROUND);
            Expect(StatBlock);
        } else if (token.type == RETURN) {
            Expect(Exp);
            tokenizer.EatToken(SEM);
        } else {
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
        tokenizer.EatToken(L_ROUND);
        token = tokenizer.GetToken();
        tokenizer.BackToken();
        if (token.type != R_ROUND) {
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
            Expect(Attribute);
        }
        token = tokenizer.GetToken();
        if (token.type == FUNC) {
            Expect(Symbol);
            tokenizer.EatToken(L_ROUND);
            token = tokenizer.GetToken();
            tokenizer.BackToken();
            if (token.type != R_ROUND) {
                Expect(DefArgList);
            }
            tokenizer.EatToken(R_ROUND);
            Expect(Symbol);
            Expect(StatBlock);
        } else if (token.type == VAR) {
            Expect(DefVar);
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
        if (token.type == IMPORT) {
            Expect(String);
        } else {
            tokenizer.BackToken();
            if (token.type == L_SQUARE) {
                Expect(Attribute);
            }
            tokenizer.EatToken(CLASS);
            Expect(Symbol);
            token = tokenizer.GetToken();
            if (token.type == COLON) {
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
    }

    // Print Functions

#define PrintIndent() for (int i = 0; i < indent; i++) std::cout << "  ";
#define SkipIf(cond) if (cond) { nodes[0]->PrintAST(indent); return; }
#define PrintName() std::string name = (std::string)TypeID::get<decltype(*this)>().getName(); name = name.substr(10, name.length() - 11); std::cout << name;
#define PrintNodes() std::cout << std::endl; for (auto* ast : nodes) ast->PrintAST(indent + 1);
#define PrintToken() std::cout << " " << token.ToString();
#define PrintTokenList() for (auto t : tokenList) std::cout << " " << t.ToString();

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
        SkipIf(token.type == NONE);
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

    // Compile Functions
    Compile(Symbol) {}
    Compile(Number) {}
    Compile(String) {}
    Compile(Primary) {}
    Compile(PostFix) {}
    Compile(Unary) {}
    Compile(Cast) {}
    Compile(Mul) {}
    Compile(Add) {}
    Compile(Relation) {}
    Compile(Equality) {}
    Compile(LogicAnd) {}
    Compile(LogicOr) {}
    Compile(Cond) {}
    Compile(Const) {}
    Compile(Assign) {}
    Compile(Exp) {}
    Compile(DefVar) {}
    Compile(Statement) {}
    Compile(StatBlock) {}
    Compile(DefArg) {}
    Compile(DefArgList) {}
    Compile(ArgsList) {}
    Compile(LambdaDef) {}
    Compile(ClassDef) {}
    Compile(IDList) {}
    Compile(SingleAttr) {}
    Compile(Attribute) {}
    Compile(Preprocess) {}
}}
