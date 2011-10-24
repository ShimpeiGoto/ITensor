#include "mps.h"
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using std::map;
using std::string;

template <class Tensor>
Tensor& MPSt<Tensor>::
setU(int i, Direction dir) //set unitary
    {
    if(dir == Fromleft)
        {
        if(i == left_orth_lim+1) left_orth_lim = i;
        else Error("left_orth_lim not at i-1");
        }
    else if(dir == Fromright)
        {
        if(i == right_orth_lim-1) right_orth_lim = i;
        else Error("right_orth_lim not at i-1");
        }
    return GET(A,i);
    }
template
ITensor& MPSt<ITensor>::setU(int i, Direction dir);
template
IQTensor& MPSt<IQTensor>::setU(int i, Direction dir);

template <class Tensor>
void MPSt<Tensor>::
read(std::istream& s)
    {
    if(model_ == 0)
        Error("Can't read to default constructed MPS");
    for(int j = 1; j <= N; ++j) 
        A.at(j).read(s);
    s.read((char*) &left_orth_lim,sizeof(left_orth_lim));
    s.read((char*) &right_orth_lim,sizeof(right_orth_lim));
    svd_.read(s);
    }
template
void MPSt<ITensor>::read(std::istream& s);
template
void MPSt<IQTensor>::read(std::istream& s);


template <class Tensor>
void MPSt<Tensor>::
write(std::ostream& s) const
    {
    for(int j = 1; j <= N; ++j) 
        A.at(j).write(s);
    s.write((char*) &left_orth_lim,sizeof(left_orth_lim));
    s.write((char*) &right_orth_lim,sizeof(right_orth_lim));
    svd_.write(s);
    }
template
void MPSt<ITensor>::write(std::ostream& s) const;
template
void MPSt<IQTensor>::write(std::ostream& s) const;


template <class Tensor>
void MPSt<Tensor>::
new_tensors(std::vector<ITensor>& A_)
    {
        std::vector<Index> a(N+1);
        for(int i = 1; i <= N; ++i)
        { a[i] = Index(nameint("a",i)); }
        A_[1] = ITensor(si(1),a[1]);
        for(int i = 2; i < N; i++)
        { A_[i] = ITensor(conj(a[i-1]),si(i),a[i]); }
        A_[N] = ITensor(conj(a[N-1]),si(N));
    }
template
void MPSt<ITensor>::new_tensors(std::vector<ITensor>& A_);
template
void MPSt<IQTensor>::new_tensors(std::vector<ITensor>& A_);

template <class Tensor>
void MPSt<Tensor>::
random_tensors(std::vector<ITensor>& A_)
    { 
    new_tensors(A_); 
    for(int i = 1; i <= N; ++i)
        A_[i].Randomize(); 
    }
template
void MPSt<ITensor>::random_tensors(std::vector<ITensor>& A_);
template
void MPSt<IQTensor>::random_tensors(std::vector<ITensor>& A_);

template <class Tensor>
int MPSt<Tensor>::
averageM() const
    {
    Real avgm = 0;
    for(int b = 1; b < NN(); ++b)
        avgm += LinkInd(b).m();
    avgm /= (NN()-1);
    return (int)avgm;
    }
template
int MPSt<ITensor>::averageM() const;
template
int MPSt<IQTensor>::averageM() const;


void 
plussers(const Index& l1, const Index& l2, Index& sumind, 
          ITensor& first, ITensor& second)
    {
    sumind = Index(sumind.rawname(),l1.m()+l2.m(),sumind.type());
    first = ITensor(l1,sumind,1);
    second = ITensor(l2,sumind);
    for(int i = 1; i <= l2.m(); ++i) second(l2(i),sumind(l1.m()+i)) = 1;
    }

void 
plussers(const IQIndex& l1, const IQIndex& l2, IQIndex& sumind, 
         IQTensor& first, IQTensor& second)
    {
    map<Index,Index> l1map, l2map;
    vector<inqn> iq;
    foreach(const inqn& x, l1.iq())
        {
        Index ii = x.index;
        Index jj(ii.name(),ii.m(),ii.type());
        l1map[ii] = jj;
        iq.push_back(inqn(jj,x.qn));
        }
    foreach(const inqn& x, l2.iq())
        {
        Index ii = x.index;
        Index jj(ii.name(),ii.m(),ii.type());
        l2map[ii] = jj;
        iq.push_back(inqn(jj,x.qn));
        }
    sumind = IQIndex(sumind,iq);
    first = IQTensor(conj(l1),sumind);
    foreach(const inqn& x, l1.iq())
        {
        Index il1 = x.index;
        Index s1 = l1map[il1];
        ITensor t(il1,s1,1.0);
        first += t;
        }
    second = IQTensor(conj(l2),sumind);
    foreach(const inqn& x, l2.iq())
        {
        Index il2 = x.index;
        Index s2 = l2map[il2];
        ITensor t(il2,s2,1.0);
        second += t;
        }
    }

#define NEW_MPS_ADDITION

#ifdef NEW_MPS_ADDITION

template <>
MPSt<IQTensor>& MPSt<IQTensor>::operator+=(const MPSt<IQTensor>& other)
    {
    primelinks(0,4);

    //Create new link indices
    vector<IQIndex> nlinks(N);
    for(int b = 1; b < N; ++b)
        {
        IQIndex l1 = this->LinkInd(b);
        IQIndex l2 = other.LinkInd(b);
        vector<inqn> iq(l1.iq());
        iq.insert(iq.begin(),l2.iq().begin(),l2.iq().end());
        nlinks.at(b) = IQIndex(l2,iq);
        }
    //Create new A tensors
    vector<IQTensor> nA(N+1);
    nA[1] = IQTensor(si(1),nlinks[1]);
    for(int j = 2; j < N; ++j)
        nA[j] = IQTensor(conj(nlinks[j-1]),si(j),nlinks[j]);
    nA[N] = IQTensor(conj(nlinks[N-1]),si(N));

    for(int j = 1; j <= N; ++j)
        {
        for(IQTensor::const_iten_it 
            it = AA(j).const_iten_begin(); 
            it != AA(j).const_iten_end();
            ++it)
            { nA[j].insert(*it); }
        for(IQTensor::const_iten_it 
            it = other.AA(j).const_iten_begin(); 
            it != other.AA(j).const_iten_end();
            ++it)
            { nA[j].insert(*it); }
        }

    A.swap(nA);

    orthogonalize();

    return *this;
    }

template <class Tensor>
MPSt<Tensor>& MPSt<Tensor>::operator+=(const MPSt<Tensor>& other)
    {
    primelinks(0,4);

    vector<Tensor> first(N), second(N);
    for(int i = 1; i < N; ++i)
        {
        IndexT l1 = this->RightLinkInd(i);
        IndexT l2 = other.RightLinkInd(i);
        IndexT r(l1.rawname());
        plussers(l1,l2,r,first[i],second[i]);
        }

    AAnc(1) = AA(1) * first[1] + other.AA(1) * second[1];
    for(int i = 2; i < N; ++i)
        {
        AAnc(i) = conj(first[i-1]) * AA(i) * first[i] 
                  + conj(second[i-1]) * other.AA(i) * second[i];
        }
    AAnc(N) = conj(first[N-1]) * AA(N) + conj(second[N-1]) * other.AA(N);

    noprimelink();

    //cerr << "WARNING: skipping orthogonalize in operator+=\n";
    orthogonalize();

    return *this;
    }
template
MPSt<ITensor>& MPSt<ITensor>::operator+=(const MPSt<ITensor>& other);

#else
template <class Tensor>
MPSt<Tensor>& MPSt<Tensor>::operator+=(const MPSt<Tensor>& other)
    {
    primelinks(0,4);

    vector<Tensor> first(N), second(N);
    for(int i = 1; i < N; ++i)
        {
        IndexT l1 = this->RightLinkInd(i);
        IndexT l2 = other.RightLinkInd(i);
        IndexT r(l1.rawname());
        plussers(l1,l2,r,first[i],second[i]);
        }

    AAnc(1) = AA(1) * first[1] + other.AA(1) * second[1];
    for(int i = 2; i < N; ++i)
        {
        AAnc(i) = conj(first[i-1]) * AA(i) * first[i] 
                  + conj(second[i-1]) * other.AA(i) * second[i];
        }
    AAnc(N) = conj(first[N-1]) * AA(N) + conj(second[N-1]) * other.AA(N);

    noprimelink();

    //cerr << "WARNING: skipping orthogonalize in operator+=\n";
    orthogonalize();

    return *this;
    }
template
MPSt<ITensor>& MPSt<ITensor>::operator+=(const MPSt<ITensor>& other);
template
MPSt<IQTensor>& MPSt<IQTensor>::operator+=(const MPSt<IQTensor>& other);
#endif

/* getCenterMatrix:
 * 
 *                    s                   s
 *                    |                   |
 * Decomposes A = -<--A-->- bond into -<--U-<-- -<--Lambda-->- bond
 *
 * A is replaced with the unitary U and Lambda is diagonal.
 * If A is the OC of an MPS, Lambda will contain the Schmidt weights. 
 *
 */
 /*
void getCenterMatrix(ITensor& A, const Index& bond, Real cutoff,int minm, int maxm, ITensor& Lambda, string newbondname = "")
{
    //Create combiner
    Combiner comb;
    foreach(const Index& i, A.indexn())
    if(!(i == bond || i == IndReIm))
    { 
        comb.addleft(i); 
    }
    foreach(const Index& i, A.index1())
    if(!(i == bond || i == IndReIm ))
    { 
        comb.addleft(i); 
    }
    comb.init("combined");
    Index active = comb.right();

    //Apply combiner....
    //comb.init(active);
    ITensor Ac = comb * A;

    ITensor rho;
    if(Ac.is_complex())
    {
        ITensor re,im;
        Ac.SplitReIm(re,im);
        ITensor rec = conj(re), imc = conj(im);
        rec.primeind(active);
        rho = re * rec;
        imc.primeind(active);
        rho += im * imc;
    }
    else rho = Ac * primeind(Ac,active);
    assert(rho.r() == 2);

    //Diagonalize & truncate the density matrix
    ITensor Uc; Vector D; diag_denmat(rho,cutoff,minm,maxm,Uc,D);

    Lambda = conj(Uc) * Ac;
    A = Uc * comb; //should be conj(comb) with arrows

}
*/

//Auxilary method for convertToIQ
int collapseCols(const Vector& Diag, Matrix& M)
    {
    int nr = Diag.Length(), nc = int(Diag.sumels());
    assert(nr != 0);
    if(nc == 0) return nc;
    M = Matrix(nr,nc); M = 0;
    int c = 0;
    for(int r = 1; r <= nr; ++r)
    if(Diag(r) == 1) { M(r,++c) = 1; }
    return nc;
    }

void convertToIQ(const BaseModel& model, const vector<ITensor>& A, vector<IQTensor>& qA, QN totalq, Real cut)
    {
    const int N = A.size()-1;
    qA.resize(A.size());
    const bool is_mpo = A[1].hasindex(model.si(1).primed());
    const int Dim = model.dim();
    const int PDim = (is_mpo ? Dim : 1);

    vector<IQIndex> linkind(N);

    typedef map<QN,Vector>::value_type qD_vt;
    map<QN,Vector> qD; //Diags of compressor matrices by QN
    typedef map<QN,vector<ITensor> >::value_type qt_vt;
    map<QN,vector<ITensor> > qt; //ITensor blocks by QN
    typedef map<QN,ITensor>::value_type qC_vt;
    map<QN,ITensor> qC; //Compressor ITensors by QN
    ITensor block;
    vector<ITensor> nblock;
    vector<inqn> iq;

    QN q;

    qC[totalq] = ITensor(); //Represents Virtual index
    //First value of prev_q below set to totalq

    const int show_s = 0;

    Index bond, prev_bond;
    for(int s = 1; s <= N; ++s)
        {
        qD.clear(); qt.clear();
        if(s > 1) prev_bond = index_in_common(A[s-1],A[s],Link);
        if(s < N) bond = index_in_common(A[s],A[s+1],Link);

        if(s == show_s) { PrintDat(A[s]); }

        foreach(const qC_vt& x, qC) 
        for(int n = 1; n <= Dim;  ++n)
        for(int u = 1; u <= PDim; ++u)
            {
            //Each compressor represents a particular
            //QN channel given by prev_q
            const QN& prev_q = x.first; 
            //Alias previous compressor ITensor to comp
            const ITensor& comp = x.second; 

            q = (is_mpo ? prev_q+model.si(s).qn(n)-model.si(s).qn(u) 
                        : prev_q-model.si(s).qn(n));

            //For the last site, only keep blocks 
            //compatible with specified totalq i.e. q=0 here
            if(s == N && q != QN()) continue;

            //Set Site indices of A[s] and its previous Link Index
            block = A[s];
            if(s != 1) block *= conj(comp);
            block *= Index(model.si(s))(n);
            if(is_mpo) block *= Index(model.siP(s))(u);

            //Initialize D Vector (D records which values of
            //the right Link Index to keep for the current QN q)
            int count = qD.count(q);
            Vector& D = qD[q];
            if(count == 0) { D.ReDimension(bond.m()); D = 0; }

            if(s == show_s)
                {
                cerr << boost::format("For n = %d\n")%n;
                cerr << boost::format("Got a block with norm %.10f\n")%block.norm();
                cerr << boost::format("bond.m() = %d\n")%bond.m();
                PrintDat(block);
                if(s != 1) PrintDat(comp);
                }

            bool keep_block = false;
            if(s == N) 
                { keep_block = true; }
            else
                {
                if(bond.m() == 1 && block.norm() != 0) 
                    { D = 1; keep_block = true; }
                else
                    {
                    ITensor summed_block;
                    if(s==1) 
                        { summed_block = block; }
                    else
                        {
                        //Here we sum over the previous link index
                        //which is already ok, analyze the one to the right
                        assert(comp.r()==2);
                        Index new_ind = (comp.index(1)==prev_bond ? comp.index(2) : comp.index(1));
                        summed_block = ITensor(new_ind,1) * block;
                        }
                    //cerr << boost::format("s = %d, bond=")%s << bond << "\n";
                    //summed_block.print("summed_block");

                    Real rel_cut = -1;
                    for(int j = 1; j <= bond.m(); ++j)
                        { rel_cut = max(fabs(summed_block.val1(j)),rel_cut); }
                    assert(rel_cut >= 0);
                    //Real rel_cut = summed_block.norm()/summed_block.vec_size();
                    rel_cut *= cut;
                    //cerr << "rel_cut == " << rel_cut << "\n";

                    if(rel_cut > 0)
                    for(int j = 1; j <= bond.m(); ++j)
                        {
                        if(fabs(summed_block.val1(j)) > rel_cut) 
                            { D(j) = 1; keep_block = true; }
                        }
                    }
                } //else (s != N)

            if(keep_block)
                {
                qD[q] = D;

                if(is_mpo) 
                    {
                    block.addindex1(conj(model.si(s)(n).index()));
                    block.addindex1(model.siP(s)(u).index());
                    }
                else 
                    { block.addindex1(model.si(s)(n).index()); }

                qt[q].push_back(block);

                if(s==show_s)
                    {
                    block.print("Kept block",ShowData);
                    cerr << "D = " << D << "\n";
                    }
                }
            }

        qC.clear();

        foreach(const qt_vt& x, qt)
            {
            const vector<ITensor>& blks = x.second;
            if(blks.size() != 0)
                {
                q = x.first; 
                if(s == N) 
                    { foreach(const ITensor& t, blks) nblock.push_back(t); }
                else
                    {
                    Matrix M; int mm = collapseCols(qD[q],M);
                    if(s==show_s)
                        {
                        cerr << boost::format("Adding block, mm = %d\n")%mm;
                        q.print("q");
                        cerr << "qD[q] = " << qD[q] << "\n";
                        cerr << "M = \n" << M << "\n";
                        int count = 0;
                        foreach(const ITensor& t, blks) 
                        t.print((boost::format("t%02d")%(++count)).str(),ShowData);
                        }
                    //string qname = (boost::format("ql%d(%+d:%d:%s)")%s%q.sz()%q.Nf()%(q.Nfp() == 0 ? "+" : "-")).str();
                    string qname = (boost::format("ql%d(%+d:%d)")%s%q.sz()%q.Nf()).str();
                    Index qbond(qname,mm);
                    ITensor compressor(bond,qbond,M);
                    foreach(const ITensor& t, blks) nblock.push_back(t * compressor);
                    iq.push_back(inqn(qbond,q));
                    qC[q] = compressor;
                    }
                }
            }

        if(s != N) 
            { 
            if(iq.empty()) 
                {
                cerr << "At site " << s << "\n";
                Error("convertToIQ: no compatible QNs to put into Link.");
                }
            linkind[s] = IQIndex(nameint("qL",s),iq); iq.clear(); 
            }
        if(s == 1)
            {
            qA[s] = (is_mpo ? IQTensor(conj(model.si(s)),model.siP(s),linkind[s]) 
                            : IQTensor(model.si(s),linkind[s]));
            }
        else if(s == N)
            {
            qA[s] = (is_mpo ? IQTensor(conj(linkind[s-1]),conj(model.si(s)),model.siP(s)) 
                            : IQTensor(conj(linkind[s-1]),model.si(s)));
            }
        else
            {
            qA[s] = (is_mpo ? IQTensor(conj(linkind[s-1]),conj(model.si(s)),model.siP(s),linkind[s]) 
                            : IQTensor(conj(linkind[s-1]),model.si(s),linkind[s]));
            }

        foreach(const ITensor& nb, nblock) 
            { qA[s] += nb; } 
        nblock.clear();

        if(s==show_s)
            {
            qA[s].print((boost::format("qA[%d]")%s).str(),ShowData);
            Error("Stopping");
            }

        } //for loop over s

    } //void convertToIQ

/*
template <class Tensor> 
template <class IQMPSType> 
void MPSt<Tensor>::convertToIQ(IQMPSType& iqpsi, QN totalq, Real cut) const
{
    assert(model_ != 0);
    const ModelT& sst = *model_;

    iqpsi = IQMPSType(sst,maxm,cutoff);

    if(!A[1].hasindex(si(1))) Error("convertToIQ: incorrect primelevel for conversion");
    bool is_mpo = A[1].hasindex(si(1).primed());
    const int Dim = si(1).m();
    const int PDim = (is_mpo ? Dim : 1);

    vector<IQIndex> linkind(N);

    typedef map<QN,Vector>::value_type qD_vt;
    map<QN,Vector> qD; //Diags of compressor matrices by QN
    typedef map<QN,vector<ITensor> >::value_type qt_vt;
    map<QN,vector<ITensor> > qt; //ITensor blocks by QN
    typedef map<QN,ITensor>::value_type qC_vt;
    map<QN,ITensor> qC; //Compressor ITensors by QN
    ITensor block;
    vector<ITensor> nblock;
    vector<inqn> iq;

    QN q;

    qC[totalq] = ITensor(); //Represents Virtual index
    //First value of prev_q below set to totalq

    const int show_s = 0;

    Index bond, prev_bond;
    for(int s = 1; s <= N; ++s)
    {

        qD.clear(); qt.clear();
        if(s > 1) prev_bond = LinkInd(s-1); 
        if(s < N) bond = LinkInd(s);

        if(s == show_s) 
        {
            PrintDat(A[s]);
        }

        foreach(const qC_vt& x, qC) {
        const QN& prev_q = x.first; const ITensor& comp = x.second; 
        for(int n = 1; n <= Dim;  ++n)
        for(int u = 1; u <= PDim; ++u)
        {
            q = (is_mpo ? prev_q+si(s).qn(n)-si(s).qn(u) : prev_q-si(s).qn(n));

            //For the last site, only keep blocks 
            //compatible with specified totalq i.e. q=0 here
            if(s == N && q != QN()) continue;

            //Set Site indices of A[s] and its previous Link Index
            block = A[s];
            if(s != 1) block *= conj(comp);
            block *= si(s)(n);
            if(is_mpo) block *= siP(s)(u);

            //Initialize D Vector (D records which values of
            //the right Link Index to keep for the current QN q)
            int count = qD.count(q);
            Vector& D = qD[q];
            if(count == 0) { D.ReDimension(bond.m()); D = 0; }

            if(s == show_s)
            {
                cerr << boost::format("For n = %d\n")%n;
                cerr << boost::format("Got a block with norm %.10f\n")%block.norm();
                cerr << boost::format("bond.m() = %d\n")%bond.m();
                PrintDat(block);
                if(s != 1) PrintDat(comp);
            }

            bool keep_block = false;
            if(s == N) keep_block = true;
            else
            {
                if(bond.m() == 1 && block.norm() != 0) { D = 1; keep_block = true; }
                else
                {
                    ITensor summed_block;
                    if(s==1) summed_block = block;
                    else
                    {
                        //Here we sum over the previous link index
                        //which is already ok, analyze the one to the right
                        assert(comp.r()==2);
                        Index new_ind = (comp.index(1)==prev_bond ? comp.index(2) : comp.index(1));
                        summed_block = ITensor(new_ind,1) * block;
                    }
                    //cerr << boost::format("s = %d, bond=")%s << bond << "\n";
                    //summed_block.print("summed_block");

                    Real rel_cut = -1;
                    for(int j = 1; j <= bond.m(); ++j)
                    { rel_cut = max(fabs(summed_block.val1(j)),rel_cut); }
                    assert(rel_cut >= 0);
                    //Real rel_cut = summed_block.norm()/summed_block.vec_size();
                    rel_cut *= cut;
                    //cerr << "rel_cut == " << rel_cut << "\n";

                    if(rel_cut > 0)
                    for(int j = 1; j <= bond.m(); ++j)
                    if(fabs(summed_block.val1(j)) > rel_cut) 
                    { D(j) = 1; keep_block = true; }
                }
            } //else (s != N)

            //if(!keep_block && q == totalq) { D(1) = 1; keep_block = true; }

            if(keep_block)
            {
                qD[q] = D;

                if(is_mpo) 
                {
                block.addindex1(conj(si(s)(n).index()));
                block.addindex1(siP(s)(u).index());
                }
                else { block.addindex1(si(s)(n).index()); }

                qt[q].push_back(block);

                if(s==show_s)
                {
                block.print("Kept block",ShowData);
                cerr << "D = " << D << "\n";
                }
            }
        }}

        qC.clear();

        foreach(const qt_vt& x, qt)
        {
            const vector<ITensor>& blks = x.second;
            if(blks.size() != 0)
            {
                q = x.first; 
                if(s == N) 
                { foreach(const ITensor& t, blks) nblock.push_back(t); }
                else
                {
                    Matrix M; int mm = collapseCols(qD[q],M);
                    if(s==show_s)
                    {
                        cerr << boost::format("Adding block, mm = %d\n")%mm;
                        q.print("q");
                        cerr << "qD[q] = " << qD[q] << "\n";
                        cerr << "M = \n" << M << "\n";
                        int count = 0;
                        foreach(const ITensor& t, blks) 
                        t.print((boost::format("t%02d")%(++count)).str(),ShowData);
                    }
                    //string qname = (boost::format("ql%d(%+d:%d:%s)")%s%q.sz()%q.Nf()%(q.Nfp() == 0 ? "+" : "-")).str();
                    string qname = (boost::format("ql%d(%+d:%d)")%s%q.sz()%q.Nf()).str();
                    Index qbond(qname,mm);
                    ITensor compressor(bond,qbond,M);
                    foreach(const ITensor& t, blks) nblock.push_back(t * compressor);
                    iq.push_back(inqn(qbond,q));
                    qC[q] = compressor;
                }
            }
        }

        if(s != N) 
        { 
            if(iq.empty()) 
            {
                cerr << "At site " << s << "\n";
                Error("convertToIQ: no compatible QNs to put into Link.");
            }
            linkind[s] = IQIndex(nameint("qL",s),iq); iq.clear(); 
        }
        if(s == 1)
        {
            iqpsi.AAnc(s) = (is_mpo ? IQTensor(conj(si(s)),siP(s),linkind[s]) : IQTensor(si(s),linkind[s]));
        }
        else if(s == N)
        {
            iqpsi.AAnc(s) = (is_mpo ? IQTensor(conj(linkind[s-1]),conj(si(s)),siP(s)) 
                                    : IQTensor(conj(linkind[s-1]),si(s)));
        }
        else
        {
            iqpsi.AAnc(s) = (is_mpo ? IQTensor(conj(linkind[s-1]),conj(si(s)),siP(s),linkind[s]) 
                                    : IQTensor(conj(linkind[s-1]),si(s),linkind[s]));
        }

        foreach(const ITensor& nb, nblock) { iqpsi.AAnc(s) += nb; } nblock.clear();

        if(0) //try to get this working ideally
        if(!is_mpo && s > 1) 
        {
            IQTensor AA = iqpsi.bondTensor(s-1);
            iqpsi.doSVD(s-1,AA,Fromleft);
        }

        if(s==show_s)
        {
        iqpsi.AA(s).print((boost::format("qA[%d]")%s).str(),ShowData);
        Error("Stopping");
        }

    } //for loop over s

    assert(checkQNs(iqpsi));

} //void convertToIQ(IQMPSType& iqpsi) const
*/

int 
findCenter(const IQMPS& psi)
    {
    for(int j = 1; j <= psi.NN(); ++j) 
        {
        const IQTensor& A = psi.AA(j);
        if(A.r() == 0) Error("Zero rank tensor in MPS");
        bool allSameDir = true;
        Arrow dir = A.index(1).dir();
        for(int i = 2; i <= A.r(); ++i)
            if(A.index(i).dir() != dir)
            {
                allSameDir = false;
                break;
            }

        //Found the ortho. center
        if(allSameDir) return j;
        }
    return -1;
    }

bool 
checkQNs(const IQMPS& psi)
    {
    const int N = psi.NN();

    QN Zero;

    int center = findCenter(psi);
    if(center == -1)
        {
        std::cerr << "Did not find an ortho. center\n";
        return false;
        }

    //Check that all IQTensors have zero div
    //except possibly the ortho. center
    for(int i = 1; i <= N; ++i) 
        {
        if(i == center) continue;
        if(psi.AA(i).is_null())
            {
            std::cerr << boost::format("AA(%d) null, QNs not well defined\n")%i;
            return false;
            }
        if(psi.AA(i).div() != Zero)
            {
            std::cerr << "At i = " << i << "\n";
            Print(psi.AA(i));
            std::cerr << "IQTensor other than the ortho center had non-zero divergence\n";
            return false;
            }
        }

    //Check arrows from left edge
    for(int i = 1; i < center; ++i)
        {
        if(psi.RightLinkInd(i).dir() != In) 
            {
            std::cerr << boost::format("checkQNs: At site %d to the left of the OC, Right side Link not pointing In\n")%i;
            return false;
            }
        if(i > 1)
            {
            if(psi.LeftLinkInd(i).dir() != Out) 
                {
                std::cerr << boost::format("checkQNs: At site %d to the left of the OC, Left side Link not pointing Out\n")%i;
                return false;
                }
            }
        }

    //Check arrows from right edge
    for(int i = N; i > center; --i)
        {
        if(i < N)
        if(psi.RightLinkInd(i).dir() != Out) 
            {
            std::cerr << boost::format("checkQNs: At site %d to the right of the OC, Right side Link not pointing Out\n")%i;
            return false;
            }
        if(psi.LeftLinkInd(i).dir() != In) 
            {
            std::cerr << boost::format("checkQNs: At site %d to the right of the OC, Left side Link not pointing In\n")%i;
            return false;
            }
        }

    //Done checking arrows
    return true;
    }

void 
fitWF(const IQMPS& psi_basis, IQMPS& psi_to_fit)
    {
    if(!psi_basis.is_ortho()) Error("psi_basis must be orthogonolized.");
    if(psi_basis.ortho_center() != 1) Error("psi_basis must be orthogonolized to site 1.");


    int N = psi_basis.NN();
    if(psi_to_fit.NN() != N) Error("Wavefunctions must have same number of sites.");

    IQTensor A = psi_to_fit.AA(N) * conj(primelink(psi_basis.AA(N)));
    for(int n = N-1; n > 1; --n)
        {
        A *= conj(primelink(psi_basis.AA(n)));
        A *= psi_to_fit.AA(n);
        }
    A = psi_to_fit.AA(1) * A;
    A.noprime();

    psi_to_fit = psi_basis;
    psi_to_fit.AAnc(1) = A;
    }
