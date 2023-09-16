#include <cstdio>

//////////////////////////////////////////////////////////////////////////////
//
// Code for interpreting Brainfuck code using C++ templates
// 
//////////////////////////////////////////////////////////////////////////////
////// Godbolt: https://godbolt.org/z/v4nz3sozE
////// Brainfuck: https://en.wikipedia.org/wiki/Brainfuck
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Tape

template <typename T, T... cs>
struct Reverse;

template <char... Cs>
struct Tape
{
    using type = Tape<Cs...>;
    using rev = Reverse<char, Cs...>;

    static constexpr char cstr[] = {Cs..., '\0'};
    static constexpr bool isEmpty = false;
};

template <>
struct Tape<>
{
    using type = Tape<>;
    using rev = Reverse<char>;

    static constexpr char cstr[] = "";
    static constexpr bool isEmpty = true;
};

namespace RevImplNS
{
    template <typename SRC, typename DEST>
    struct ReverseImpl;

    template <char... DESTs>
    struct ReverseImpl<Tape<>, Tape<DESTs...>>
    {
        using second = Tape<DESTs...>;
    };

    template <char C, char... SRCs, char... DESTs>
    struct ReverseImpl<Tape<C, SRCs...>, Tape<DESTs...>> : ReverseImpl<Tape<SRCs...>, Tape<C, DESTs...>>
    {
    };

} // namespace RevImplNS

template <char... Cs>
struct Reverse<char, Cs...> : RevImplNS::ReverseImpl<Tape<Cs...>, Tape<>>::second
{
};

template <typename... Ts>
struct Concat;

template <char... C1s, char... C2s>
struct Concat<Tape<C1s...>, Tape<C2s...>> : Tape<C1s..., C2s...>
{
};

template <char... C1s, char... C2s, char... C3s>
struct Concat<Tape<C1s...>, Tape<C2s...>, Tape<C3s...>> : Tape<C1s..., C2s..., C3s...>
{
};

template <char... C1s, char... C2s, char... C3s, char... C4s>
struct Concat<Tape<C1s...>, Tape<C2s...>, Tape<C3s...>, Tape<C4s...>> : Tape<C1s..., C2s..., C3s..., C4s...>
{
};

template <char... C1s, char... C2s, char... C3s, char... C4s, char... C5s>
struct Concat<Tape<C1s...>, Tape<C2s...>, Tape<C3s...>, Tape<C4s...>, Tape<C5s...>> : Tape<C1s..., C2s..., C3s..., C4s..., C5s...>
{
};

namespace UByteToTapeImplNS
{
    template <unsigned char FstDigits, unsigned char LstDigit, typename Tape>
    struct UByteToTapeImpl;

    template <unsigned char FstDigits, unsigned char LstDigit, char... Cs>
    struct UByteToTapeImpl<FstDigits, LstDigit, Tape<Cs...>>
        : UByteToTapeImpl<FstDigits / 10, FstDigits % 10, Tape<(char)('0' + LstDigit), Cs...>>
    {
    };

    template <unsigned char LstDigit, char... Cs>
    struct UByteToTapeImpl<0, LstDigit, Tape<Cs...>>
    {
        using tape = Tape<(char)('0' + LstDigit), Cs...>;
    };
} // namespace IntToTapeImplNS

template <unsigned char N>
struct UByteToTape : UByteToTapeImplNS::UByteToTapeImpl<N / 10, N % 10, Tape<>>::tape
{
};

template <typename TAPE, typename T, template <T t> typename FUNC>
struct MapReduce;

template <typename T, template <T t> typename FUNC>
struct MapReduce<Tape<>, T, FUNC> : Tape<>
{
};

template <char C, char... Cs, typename T, template <T t> typename FUNC>
struct MapReduce<Tape<C, Cs...>, T, FUNC>
    : Concat<
          typename FUNC<(T)C>::type,
          typename MapReduce<Tape<Cs...>, T, FUNC>::type>
{
};

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// IndexedStorage
template <typename L, char C, typename R>
struct IndexedStorage;

template <char... Ls, char C, char... Rs>
struct IndexedStorage<Tape<Ls...>, C, Tape<Rs...>>
{
    using type = IndexedStorage<Tape<Ls...>, C, Tape<Rs...>>;
    using lTape = Tape<Ls...>;
    using rTape = Tape<Rs...>;
    static constexpr char curr = C;

    static constexpr bool isFirstChar = lTape::isEmpty;
    static constexpr bool isLastChar = rTape::isEmpty;

    using asTape = typename Concat<typename Tape<Ls...>::rev::type, Tape<C, Rs...>>::type;
    using asDBGTape = typename Concat<typename Tape<Ls...>::rev::type, Tape<'|', C, '|', Rs...>>::type;
};

template <char C, char... Cs>
struct InitIndexedStorage : IndexedStorage<Tape<>, C, Tape<Cs...>>
{
};
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Program Storage
template <typename S>
struct ProgramStorage;

namespace PrgStorageNS
{
    template <typename PS>
    struct ShiftR;

    template <typename PS>
    struct ShiftL;
} // namespace PrgStorageNS

template <typename LTape, char C, typename RTape>
struct ProgramStorage<IndexedStorage<LTape, C, RTape>> : IndexedStorage<LTape, C, RTape>
{
    using storageType = IndexedStorage<LTape, C, RTape>;
    using type = ProgramStorage<storageType>;
    using next = PrgStorageNS::ShiftR<type>;
    using prev = PrgStorageNS::ShiftL<type>;
};

template <char C, char... Cs>
struct InitProgramStorage : ProgramStorage<IndexedStorage<Tape<>, C, Tape<Cs...>>>
{
};

namespace PrgStorageNS
{
    template <char... Ls, char COld, char CNew, char... Rs>
    struct ShiftR<
        ProgramStorage<
            IndexedStorage<Tape<Ls...>, COld, Tape<CNew, Rs...>>>>
        : ProgramStorage<
              IndexedStorage<Tape<COld, Ls...>, CNew, Tape<Rs...>>>
    {
    };

    template <char... Ls, char COld, char CNew, char... Rs>
    struct ShiftL<
        ProgramStorage<
            IndexedStorage<Tape<CNew, Ls...>, COld, Tape<Rs...>>>>
        : ProgramStorage<
              IndexedStorage<Tape<Ls...>, CNew, Tape<COld, Rs...>>>
    {
    };
} // namespace PrgStorageNS

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Memory Storage
template <typename S>
struct MemoryStorage;

namespace MemStorageNS
{
    template <typename MS>
    struct MoveR;

    template <typename MS>
    struct MoveL;

    template <typename MS>
    struct Inc;

    template <typename MS>
    struct Dec;
} // namespace MemStorageNS

template <typename LTape, char C, typename RTape>
struct MemoryStorage<IndexedStorage<LTape, C, RTape>> : IndexedStorage<LTape, C, RTape>
{
    using storageType = IndexedStorage<LTape, C, RTape>;
    using type = MemoryStorage<storageType>;
    using moveR = MemStorageNS::MoveR<type>;
    using moveL = MemStorageNS::MoveL<type>;
    using inc = MemStorageNS::Inc<type>;
    using dec = MemStorageNS::Dec<type>;

    using asDBGTape = typename Concat<
        typename MapReduce<typename LTape::rev::type, unsigned char, UByteToTape>::type,
        typename Concat<Tape<'|'>, typename UByteToTape<(unsigned char)C>::type, Tape<'|'>>::type,
        typename MapReduce<RTape, unsigned char, UByteToTape>::type>::type;
};

template <char C, char... Cs>
struct InitMemoryStorage : MemoryStorage<IndexedStorage<Tape<>, C, Tape<Cs...>>>
{
};

namespace MemStorageNS
{

    template <char... Ls, char COld>
    struct MoveR<
        MemoryStorage<
            IndexedStorage<Tape<Ls...>, COld, Tape<>>>>
        : MemoryStorage<
              IndexedStorage<Tape<COld, Ls...>, 0, Tape<>>>
    {
    };

    template <char... Ls, char COld, char CNew, char... Rs>
    struct MoveR<
        MemoryStorage<
            IndexedStorage<Tape<Ls...>, COld, Tape<CNew, Rs...>>>>
        : MemoryStorage<
              IndexedStorage<Tape<COld, Ls...>, CNew, Tape<Rs...>>>
    {
    };

    template <char COld, char... Rs>
    struct MoveL<
        MemoryStorage<
            IndexedStorage<Tape<>, COld, Tape<Rs...>>>>
        : MemoryStorage<
              IndexedStorage<Tape<>, 0, Tape<COld, Rs...>>>
    {
    };

    template <char... Ls, char COld, char CNew, char... Rs>
    struct MoveL<
        MemoryStorage<
            IndexedStorage<Tape<CNew, Ls...>, COld, Tape<Rs...>>>>
        : MemoryStorage<
              IndexedStorage<Tape<Ls...>, CNew, Tape<COld, Rs...>>>
    {
    };

    template <typename LTape, char C, typename RTape>
    struct Inc<MemoryStorage<IndexedStorage<LTape, C, RTape>>>
        : MemoryStorage<IndexedStorage<LTape, C + 1, RTape>>
    {
    };

    template <typename LTape, typename RTape>
    struct Inc<MemoryStorage<IndexedStorage<LTape, (char)255, RTape>>>
        : MemoryStorage<IndexedStorage<LTape, 0, RTape>>
    {
    };

    template <typename LTape, char C, typename RTape>
    struct Dec<MemoryStorage<IndexedStorage<LTape, C, RTape>>>
        : MemoryStorage<IndexedStorage<LTape, C - 1, RTape>>
    {
    };

    template <typename LTape, typename RTape>
    struct Dec<MemoryStorage<IndexedStorage<LTape, 0, RTape>>>
        : MemoryStorage<IndexedStorage<LTape, (char)255, RTape>>
    {
    };
} // namespace MemStorageNS

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Machine
template <typename PRG, typename MEMORY, typename OUT>
struct Machine;

template <typename Mch>
struct AdvanceProgramCounter;

template <typename Mch>
struct RewindProgramCounter;

template <typename Mch>
struct NextMemCell;

template <typename Mch>
struct PrevMemCell;

template <typename Mch>
struct IncMemVal;

template <typename Mch>
struct DecMemVal;

template <typename Mch>
struct PrintMemVal;

template <typename Mch>
struct MachineExecuter;

template <typename PRG_STORE, typename MEM_STORE, char... OUTs>
struct Machine<
    ProgramStorage<PRG_STORE>,
    MemoryStorage<MEM_STORE>,
    Tape<OUTs...>>
{
    using prg = ProgramStorage<PRG_STORE>;
    using mem = MemoryStorage<MEM_STORE>;
    using out = Tape<OUTs...>;
    using type = Machine<prg, mem, out>;

    using advPC = AdvanceProgramCounter<type>;
    using rewPC = RewindProgramCounter<type>;

    using nextMem = NextMemCell<type>;
    using prevMem = PrevMemCell<type>;
    using incMem = IncMemVal<type>;
    using decMem = DecMemVal<type>;
    using printMem = PrintMemVal<type>;

    using dbg_prg = typename Concat<Tape<'P', 'R', 'G', ':'>, typename prg::asDBGTape>::type;
    using dbg_mem = typename Concat<Tape<'\n', 'M', 'E', 'M', ':'>, typename mem::asDBGTape>::type;
    using dbg_out = typename Concat<Tape<'\n', 'O', 'U', 'T', ':'>, out>::type;

    using asDBGTape = typename Concat<dbg_prg, dbg_mem, dbg_out>::type;
};

template <char... PRG_IN>
struct InitMachine : Machine<typename InitProgramStorage<PRG_IN...>::type,
                             typename InitMemoryStorage<0>::type, Tape<>>
{
};

template <typename PRG_Store, typename MEM, typename OUT>
struct AdvanceProgramCounter<Machine<ProgramStorage<PRG_Store>, MEM, OUT>>
    : Machine<typename ProgramStorage<PRG_Store>::next::type, MEM, OUT>
{
};

template <typename PRG_Store, typename MEM, typename OUT>
struct RewindProgramCounter<Machine<ProgramStorage<PRG_Store>, MEM, OUT>>
    : Machine<typename ProgramStorage<PRG_Store>::prev::type, MEM, OUT>
{
};

template <typename PRG, typename MEM_Store, typename OUT>
struct NextMemCell<Machine<PRG, MemoryStorage<MEM_Store>, OUT>>
    : Machine<PRG, typename MemoryStorage<MEM_Store>::moveR::type, OUT>
{
};

template <typename PRG, typename MEM_Store, typename OUT>
struct PrevMemCell<Machine<PRG, MemoryStorage<MEM_Store>, OUT>>
    : Machine<PRG, typename MemoryStorage<MEM_Store>::moveL::type, OUT>
{
};

template <typename PRG, typename MEM_Store, typename OUT>
struct IncMemVal<Machine<PRG, MemoryStorage<MEM_Store>, OUT>>
    : Machine<PRG, typename MemoryStorage<MEM_Store>::inc::type, OUT>
{
};

template <typename PRG, typename MEM_Store, typename OUT>
struct DecMemVal<Machine<PRG, MemoryStorage<MEM_Store>, OUT>>
    : Machine<PRG, typename MemoryStorage<MEM_Store>::dec::type, OUT>
{
};

template <typename PRG, typename MEM, char... Cs>
struct PrintMemVal<Machine<PRG, MEM, Tape<Cs...>>>
    : Machine<PRG, MEM, Tape<Cs..., MEM::curr>>
{
};

namespace MchExecuterNs
{
    template <char Instruction, bool Done>
    struct State
    {
        using type = State<Instruction, Done>;
        static constexpr char inst = Instruction;
        static constexpr bool done = Done;
    };

    template <typename Mch>
    struct InitState : State<Mch::prg::curr, false>
    {
    };

    template <typename Mch, char I>
    struct MachineForInstruction;

    template <typename Mch, typename S>
    struct Impl;

    template <typename Mch, char B, char C, int N, bool End>
    struct MoveToMatchingBracketImpl;

    template <typename Mch, char B>
    struct MoveToMatchingBracket;

    template <typename Mch, char B, bool Jump>
    struct CalcBracketMoveImpl;

    template <typename Mch, char B>
    struct CalcBracketMove;

    template <typename Mch>
    struct ExecuterInit : Impl<Mch, typename InitState<Mch>::type>
    {
    };

    struct InvalidMachine : Machine<
                                InitProgramStorage<'X'>::type,
                                InitMemoryStorage<0>::type,
                                Tape<'E', 'r', 'r', 'o', 'r', '!'>>
    {
    };

    template <typename Mch>
    struct MachineForInstruction<Mch, '>'> : Mch::nextMem
    {
    };

    template <typename Mch>
    struct MachineForInstruction<Mch, '<'> : Mch::prevMem
    {
    };

    template <typename Mch>
    struct MachineForInstruction<Mch, '+'> : Mch::incMem
    {
    };

    template <typename Mch>
    struct MachineForInstruction<Mch, '-'> : Mch::decMem
    {
    };

    template <typename Mch>
    struct MachineForInstruction<Mch, '.'> : Mch::printMem
    {
    };

    template <typename Mch>
    struct MachineForInstruction<Mch, '['> : CalcBracketMove<Mch, '['>::type
    {
    };

    template <typename Mch>
    struct MachineForInstruction<Mch, ']'> : CalcBracketMove<Mch, ']'>::type
    {
    };

    //////////////////////////////////////////////////////////////////////////////

    template <typename Mch, char B>
    struct CalcBracketMoveImpl<Mch, B, true> : MoveToMatchingBracket<Mch, B>
    {
    };

    template <typename Mch, char B>
    struct CalcBracketMoveImpl<Mch, B, false> : Mch
    {
    };

    template <typename Mch>
    struct CalcBracketMove<Mch, '['> : CalcBracketMoveImpl<Mch, '[', Mch::mem::curr == 0>
    {
    };
    template <typename Mch>
    struct CalcBracketMove<Mch, ']'> : CalcBracketMoveImpl<Mch, ']', Mch::mem::curr != 0>
    {
    };

    //////////////////////////////////////////////////////////////////////////////

    template <typename Mch, char B, char C, int N>
    struct MoveToMatchingBracketImpl<Mch, B, C, N, true> : InvalidMachine
    {
    };

    template <typename Mch>
    struct MoveToMatchingBracket<Mch, '['> : MoveToMatchingBracketImpl<Mch, '[', Mch::advPC::prg::curr, 0, Mch::advPC::prg::isLastChar>
    {
    };

    template <typename Mch>
    struct MoveToMatchingBracket<Mch, ']'> : MoveToMatchingBracketImpl<Mch, ']', Mch::rewPC::prg::curr, 0, Mch::rewPC::prg::isFirstChar>
    {
    };

    //////////////////////////////////////////////////////////////////////////////

    template <typename Mch>
    struct MoveToMatchingBracketImpl<Mch, '[', ']', 0, true> : Mch
    {
    };

    template <typename Mch>
    struct MoveToMatchingBracketImpl<Mch, '[', ']', 0, false> : Mch
    {
    };

    template <typename Mch, int N>
    struct MoveToMatchingBracketImpl<Mch, '[', '[', N, false> : MoveToMatchingBracketImpl<
                                                                    typename Mch::advPC, '[',
                                                                    Mch::advPC::prg::curr, N + 1,
                                                                    Mch::advPC::prg::isLastChar>
    {
    };

    template <typename Mch, int N>
    struct MoveToMatchingBracketImpl<Mch, '[', ']', N, false> : MoveToMatchingBracketImpl<
                                                                    typename Mch::advPC, '[',
                                                                    Mch::advPC::prg::curr, N - 1,
                                                                    Mch::advPC::prg::isLastChar>
    {
    };

    template <typename Mch, char C, int N>
    struct MoveToMatchingBracketImpl<Mch, '[', C, N, false> : MoveToMatchingBracketImpl<
                                                                  typename Mch::advPC, '[',
                                                                  Mch::advPC::prg::curr, N,
                                                                  Mch::advPC::prg::isLastChar>
    {
    };

    //////////////////////////////////////////////////////////////////////////////

    template <typename Mch>
    struct MoveToMatchingBracketImpl<Mch, ']', '[', 0, true> : Mch
    {
    };

    template <typename Mch>
    struct MoveToMatchingBracketImpl<Mch, ']', '[', 0, false> : Mch
    {
    };

    template <typename Mch, int N>
    struct MoveToMatchingBracketImpl<Mch, ']', ']', N, false> : MoveToMatchingBracketImpl<
                                                                    typename Mch::rewPC, ']',
                                                                    Mch::rewPC::prg::curr, N + 1,
                                                                    Mch::rewPC::prg::isFirstChar>
    {
    };

    template <typename Mch, int N>
    struct MoveToMatchingBracketImpl<Mch, ']', '[', N, false> : MoveToMatchingBracketImpl<
                                                                    typename Mch::rewPC, ']',
                                                                    Mch::rewPC::prg::curr, N - 1,
                                                                    Mch::rewPC::prg::isFirstChar>
    {
    };

    template <typename Mch, char C, int N>
    struct MoveToMatchingBracketImpl<Mch, ']', C, N, false> : MoveToMatchingBracketImpl<
                                                                  typename Mch::rewPC, ']',
                                                                  Mch::rewPC::prg::curr, N,
                                                                  Mch::rewPC::prg::isFirstChar>
    {
    };

    //////////////////////////////////////////////////////////////////////////////

    template <typename Mch, bool Done>
    struct SelectImpl;

    template <typename Mch>
    struct SelectImpl<Mch, false> : Impl<typename Mch::advPC, State<Mch::advPC::prg::curr, false>>
    {
    };

    template <typename Mch>
    struct SelectImpl<Mch, true> : Impl<Mch, State<Mch::prg::curr, true>>
    {
    };

    template <typename Mch, char I>
    struct Impl<Mch, State<I, true>> : Mch
    {
    };

    template <typename Mch, char I>
    struct Impl<Mch, State<I, false>> : SelectImpl<typename MachineForInstruction<Mch, I>::type, MachineForInstruction<Mch, I>::type::prg::isLastChar>
    {
    };

    //////////////////////////////////////////////////////////////////////////////

} // namespace MchExecuterNs

template <typename PRG, typename MEM, typename OUT>
struct MachineExecuter<Machine<PRG, MEM, OUT>>
    : MchExecuterNs::ExecuterInit<Machine<PRG, MEM, OUT>>
{
};

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        int i = 0;
        printf("Template Packed Params:<");
        if (argv[1][i] != '\0')
        {
            printf("'%c'", argv[1][i]);
            i++;
        }
        while (argv[1][i] != '\0')
        {
            printf(",'%c'", argv[1][i]);
            i++;
        }
        printf(">\n");
        return 0;
    }

    {
        using initMachine = typename InitMachine<'+','+','+','+','+','+','+','+','[','>','+','+','+','+','[','>','+','+','>','+','+','+','>','+','+','+','>','+','<','<','<','<','-',']','>','+','>','+','>','-','>','>','+','[','<',']','<','-',']','>','>','.','>','-','-','-','.','+','+','+','+','+','+','+','.','.','+','+','+','.','>','>','.','<','-','.','<','.','+','+','+','.','-','-','-','-','-','-','.','-','-','-','-','-','-','-','-','.','>','>','+','.','>','+','+','.'>::type;
        printf("\nMachine (Prints 'Hello World!'):\n------------\n%s\n------------\n", MachineExecuter<initMachine>::asDBGTape::cstr);
    }

    {
        using initMachine = typename InitMachine<'+', '+', '+', '+', '+', '+', '+', '+', '[', '>', '+', '+', '<', '-', ']','>'>::type;
        printf("\nMachine (Calculates 8*2 in memory):\n------------\n%s\n------------\n", MachineExecuter<initMachine>::asDBGTape::cstr);
    }

    {
        using initMachine = typename InitMachine<'+','+','+','>','+','+','[','-','>','+','>','+','<','<',']','>','>','[','-','<','<','+','>','>',']','<','<','<','[','-','>','>','+','>','+','<','<','<',']','>','>','>','[','-','<','<','<','+','>','>','>',']','<'>::type;
        printf("\nMachine (Calculates 3+2 in memory):\n------------\n%s\n------------\n", MachineExecuter<initMachine>::asDBGTape::cstr);
    }
    
    {
        using initMachine = typename InitMachine<'+','+','+','[','-','>','+','+','+','[','-','>','+','+','+','+','<',']','<',']','>','>','.','>','+','+','+','[','-','>','+','+','+','[','-','>','+','+','+','[','-','>','+','+','<',']','<',']','<',']','>','>','>','.'>::type;
        printf("\nMachine (Prints:'$6'):\n------------\n%s\n------------\n", MachineExecuter<initMachine>::asDBGTape::cstr);
    }

    {
        using initMachine = typename InitMachine<'+','[','+','+','>',']','<',']'>::type;
        printf("\nMachine (Errors):\n------------\n%s\n------------\n", MachineExecuter<initMachine>::asDBGTape::cstr);
    }

    {
        using initMachine = typename InitMachine<'[','+','[','+','+',']','-'>::type;
        printf("\nMachine (Errors):\n------------\n%s\n------------\n", MachineExecuter<initMachine>::asDBGTape::cstr);
    }
}

