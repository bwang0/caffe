
load APr_fcn.mat
fcnP = myAPr_P;
fcnR = myAPr_R;


load APr_our.mat
ourP = myAPr_P;
ourR = myAPr_R;

load APr_gc.mat

figure(1); hold on;
scatter(fcnR,fcnP,1)
scatter(ourR,ourP,1)

scatter(our_50R,our_50P,1)


scatter(gc_10R,gc_10P,1)
scatter(gc_30R,gc_30P,1)
scatter(gc_50R,gc_50P,1)
scatter(gc_70R,gc_70P,1)