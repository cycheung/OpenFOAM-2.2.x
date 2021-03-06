#include "resetPhiPatches.H"

volScalarField rAU(1.0/UEqn().A());
surfaceScalarField rAUf("Dp", fvc::interpolate(rAU));

volVectorField HbyA("HbyA", U);
HbyA = rAU*UEqn().H();

if (pimple.nCorrPISO() <= 1)
{
    UEqn.clear();
}

surfaceScalarField phiHbyA
(
    "phiHbyA",
    (fvc::interpolate(HbyA) & mesh.Sf())
  + fvc::ddtPhiCorr(rAU, U, phi)
);

adjustPhi(phiHbyA, U, p_gh);

fvOptions.relativeFlux(phiHbyA);

// Non-orthogonal pressure corrector loop
while (pimple.correctNonOrthogonal())
{
    fvScalarMatrix p_ghEqn
    (
        fvm::laplacian(rAUf, p_gh) == fvc::div(phiHbyA)
    );

    p_ghEqn.setReference(p_ghRefCell, p_ghRefValue);

    p_ghEqn.solve(mesh.solver(p_gh.select(pimple.finalInnerIter())));

    if (pimple.finalNonOrthogonalIter())
    {
        phi = phiHbyA - p_ghEqn.flux();
    }
}

#include "continuityErrs.H"

// Explicitly relax pressure for momentum corrector
p_gh.relax();

p = p_gh + (g & mesh.C());

U = HbyA - rAU*fvc::grad(p_gh);
U.correctBoundaryConditions();
fvOptions.correct(U);
